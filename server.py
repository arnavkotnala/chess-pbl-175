import asyncio
import json
import websockets
import string
import random

# Dictionary to hold rooms
# room_id -> { "players": [websocket1, websocket2], "turn": 0 }
rooms = {}

def generate_room_code():
    return ''.join(random.choices(string.ascii_uppercase + string.digits, k=5))

async def handle_client(websocket, path):
    try:
        async for message in websocket:
            data = json.loads(message)
            action = data.get("action")

            if action == "create_room":
                room_id = generate_room_code()
                rooms[room_id] = {"players": [websocket]}
                await websocket.send(json.dumps({
                    "type": "room_created",
                    "room_id": room_id,
                    "color": "white"
                }))
                print(f"Room {room_id} created.")

            elif action == "join_room":
                room_id = data.get("room_id")
                if room_id in rooms:
                    if len(rooms[room_id]["players"]) < 2:
                        rooms[room_id]["players"].append(websocket)
                        await websocket.send(json.dumps({
                            "type": "room_joined",
                            "room_id": room_id,
                            "color": "black"
                        }))
                        print(f"Player joined room {room_id}.")
                        
                        # Notify the first player
                        player1 = rooms[room_id]["players"][0]
                        await player1.send(json.dumps({
                            "type": "opponent_joined"
                        }))
                    else:
                        await websocket.send(json.dumps({
                            "type": "error",
                            "message": "Room is full."
                        }))
                else:
                    await websocket.send(json.dumps({
                        "type": "error",
                        "message": "Room not found."
                    }))

            elif action == "move":
                room_id = data.get("room_id")
                move_data = data.get("move")
                
                if room_id in rooms:
                    # Forward move to the other player
                    for player in rooms[room_id]["players"]:
                        if player != websocket:
                            await player.send(json.dumps({
                                "type": "move",
                                "move": move_data
                            }))

    except websockets.ConnectionClosed:
        print("Client disconnected.")
    finally:
        # Clean up empty rooms
        empty_rooms = []
        for room_id, room_data in rooms.items():
            if websocket in room_data["players"]:
                room_data["players"].remove(websocket)
                for player in room_data["players"]:
                    asyncio.create_task(player.send(json.dumps({
                        "type": "opponent_disconnected"
                    })))
            if len(room_data["players"]) == 0:
                empty_rooms.append(room_id)
        
        for room_id in empty_rooms:
            del rooms[room_id]
            print(f"Room {room_id} deleted.")

async def main():
    print("Starting WebSocket server on port 8765...")
    async with websockets.serve(handle_client, "0.0.0.0", 8765):
        await asyncio.Future()  # run forever

if __name__ == "__main__":
    asyncio.run(main())
