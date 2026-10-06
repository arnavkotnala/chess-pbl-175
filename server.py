import os
import json
import random
import string
import asyncio
from aiohttp import web, WSMsgType

# Dictionary to hold rooms
# room_id -> { "players": [ws, ...], "white": ws, "black": ws }
rooms = {}

def generate_room_code():
    chars = string.ascii_uppercase + string.digits
    # Avoid ambiguous characters (0, O, 1, I)
    chars = chars.replace('0', '').replace('O', '').replace('1', '').replace('I', '')
    for _ in range(200):
        code = ''.join(random.choices(chars, k=5))
        if code not in rooms:
            return code
    return ''.join(random.choices(chars, k=6))

async def handle_websocket(ws):
    current_room = None
    try:
        async for msg in ws:
            if msg.type == WSMsgType.TEXT:
                try:
                    data = json.loads(msg.data)
                except Exception as e:
                    print(f"Invalid JSON received: {e}")
                    continue

                action = data.get("action")

                if action == "create_room":
                    room_id = generate_room_code()
                    # hint_count: -1=unlimited, 0=none, 3=3 hints, 5=5 hints (default 3)
                    hint_count = int(data.get("hint_count", 3))
                    if hint_count not in (-1, 0, 3, 5):
                        hint_count = 3  # sanitize
                    current_room = room_id
                    rooms[room_id] = {
                        "players": [ws],
                        "white": ws,
                        "black": None,
                        "hint_count": hint_count
                    }
                    await ws.send_str(json.dumps({
                        "type": "room_created",
                        "room_id": room_id,
                        "color": "white",
                        "hint_count": hint_count
                    }))
                    print(f"[ROOM CREATED] Code: {room_id} (Host as White, hints={hint_count})")

                elif action == "join_room":
                    room_id = str(data.get("room_id", "")).strip().upper()
                    print(f"[JOIN REQUEST] Code: '{room_id}'")
                    if room_id in rooms:
                        room_data = rooms[room_id]
                        if len(room_data["players"]) < 2:
                            current_room = room_id
                            room_data["players"].append(ws)
                            room_data["black"] = ws
                            hint_count = room_data.get("hint_count", 3)

                            # Tell joining player they are Black and the hint setting
                            await ws.send_str(json.dumps({
                                "type": "room_joined",
                                "room_id": room_id,
                                "color": "black",
                                "hint_count": hint_count
                            }))
                            print(f"[PLAYER JOINED] Room: {room_id} (Joined as Black, hints={hint_count})")

                            # Notify host (White) that opponent has joined, relay hint_count
                            host_ws = room_data["white"]
                            if host_ws and not host_ws.closed:
                                await host_ws.send_str(json.dumps({
                                    "type": "opponent_joined",
                                    "room_id": room_id,
                                    "hint_count": hint_count
                                }))
                                print(f"[GAME STARTED] Room: {room_id} - Both players connected!")
                        else:
                            await ws.send_str(json.dumps({
                                "type": "error",
                                "message": f"Room '{room_id}' is full (2 players already connected)."
                            }))
                    else:
                        await ws.send_str(json.dumps({
                            "type": "error",
                            "message": f"Room '{room_id}' not found. Check the code."
                        }))

                elif action == "move":
                    room_id = str(data.get("room_id", "")).strip().upper()
                    move_data = data.get("move")
                    if room_id in rooms:
                        for p in rooms[room_id]["players"]:
                            if p != ws and not p.closed:
                                await p.send_str(json.dumps({
                                    "type": "move",
                                    "move": move_data
                                }))

                elif action == "resign":
                    room_id = str(data.get("room_id", "")).strip().upper()
                    if room_id in rooms:
                        for p in rooms[room_id]["players"]:
                            if p != ws and not p.closed:
                                await p.send_str(json.dumps({
                                    "type": "opponent_resigned"
                                }))

                elif action == "leave_room":
                    room_id = str(data.get("room_id", "")).strip().upper()
                    if room_id in rooms:
                        if ws in rooms[room_id]["players"]:
                            rooms[room_id]["players"].remove(ws)
                        for p in rooms[room_id]["players"]:
                            if not p.closed:
                                await p.send_str(json.dumps({
                                    "type": "opponent_disconnected"
                                }))
                        if len(rooms[room_id]["players"]) == 0:
                            del rooms[room_id]
                            print(f"[ROOM CLOSED] {room_id}")
                    current_room = None

            elif msg.type in (WSMsgType.CLOSE, WSMsgType.CLOSED, WSMsgType.ERROR):
                break

    except Exception as e:
        print(f"WebSocket client error: {e}")
    finally:
        # Player disconnected or closed tab
        to_delete = []
        for r_id, r_info in list(rooms.items()):
            if ws in r_info["players"]:
                r_info["players"].remove(ws)
                print(f"[CLIENT DISCONNECTED] Left room {r_id}")
                for p in r_info["players"]:
                    if not p.closed:
                        try:
                            asyncio.create_task(p.send_str(json.dumps({
                                "type": "opponent_disconnected"
                            })))
                        except Exception:
                            pass
            if len(r_info["players"]) == 0:
                to_delete.append(r_id)

        for r_id in to_delete:
            if r_id in rooms:
                del rooms[r_id]
                print(f"[ROOM DELETED] {r_id} is empty")

async def ws_handler(request):
    ws = web.WebSocketResponse(heartbeat=30.0)
    await ws.prepare(request)
    await handle_websocket(ws)
    return ws

async def health_check(request):
    return web.json_response({
        "status": "online",
        "service": "ChessVerse 2D Online Server",
        "active_rooms": len(rooms),
        "room_ids": list(rooms.keys())
    })

async def index_handler(request):
    # Support direct WebSocket connection to root URL as well
    if request.headers.get("Upgrade", "").lower() == "websocket":
        return await ws_handler(request)
    if os.path.exists("index.html"):
        return web.FileResponse("index.html")
    return web.Response(text="ChessVerse 2D Server Online")

@web.middleware
async def no_cache_middleware(request, handler):
    response = await handler(request)
    response.headers['Cache-Control'] = 'no-cache, no-store, must-revalidate'
    response.headers['Pragma'] = 'no-cache'
    response.headers['Expires'] = '0'
    return response

def create_app():
    app = web.Application(middlewares=[no_cache_middleware])
    # WebSocket endpoints
    app.router.add_get('/ws', ws_handler)
    app.router.add_get('/health', health_check)
    app.router.add_get('/', index_handler)
    # Serve static assets (index.js, index.wasm, index.data, assets/)
    app.router.add_static('/', path='.', follow_symlinks=True)
    return app

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 8080))
    app = create_app()

    print(f"==================================================")
    print(f"   [CHESSVERSE 2D] UNIFIED CLOUD SERVER")
    print(f"   HTTP & WebSocket listening on port {port}")
    print(f"   Local URL: http://localhost:{port}")
    print(f"==================================================")

    web.run_app(app, host="0.0.0.0", port=port)
