@echo off
echo Pushing latest changes to GitHub (arnavkotnala/chess-pbl-175)...
git add .
git commit -m "Implement workable Create Room and Join Room with cloud-hosted WebSocket architecture"
git push -u origin main
echo Done!
pause
