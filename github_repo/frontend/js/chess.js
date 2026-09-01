// ═══════════════════════════════════════════════════════════════════════════
// ChessVerse — Main Game Logic (chess.js)
// Supports: AI Modes, Local Versus, and Online Multiplayer with Room Codes
// ═══════════════════════════════════════════════════════════════════════════

// ─── Dynamic API Base URL ───────────────────────────────────────────────────
function getApiUrl() {
  const custom = localStorage.getItem('chessverse_api_url');
  if (custom && custom.trim().length > 0) {
    return custom.trim().replace(/\/+$/, '');
  }
  if (window.location.protocol === 'http:' || window.location.protocol === 'https:') {
    // If served from backend server, use origin
    if (window.location.port === '8080') {
      return window.location.origin;
    }
  }
  return 'http://localhost:8080';
}

let API = getApiUrl();

function updateApiUrl(newUrl) {
  if (!newUrl) {
    localStorage.removeItem('chessverse_api_url');
  } else {
    localStorage.setItem('chessverse_api_url', newUrl);
  }
  API = getApiUrl();
}

// ─── Application State ────────────────────────────────────────────────────
let currentUser   = null;
let gameActive    = false;
let gameMode      = null;   // 'easy','medium','hard','versus','online'
let selectedSquare= null;   // { row, col } (0-7 internal coordinates)
let legalMoves    = [];     // [{ toRow, toCol }]
let boardData     = null;   // 8x8 array of piece codes
let currentTurn   = 'white';
let lastMove      = null;   // { fromRow, fromCol, toRow, toCol }
let hintHighlight = null;   // { fromRow, fromCol, toRow, toCol }
let capturedWhite = [];     // pieces captured BY white (black pieces taken)
let capturedBlack = [];     // pieces captured BY black (white pieces taken)
let whiteScore    = 0;
let blackScore    = 0;

// ─── Online Multiplayer State ─────────────────────────────────────────────
let isOnlineRoom        = false;
let currentRoomId       = null;
let myOnlineColor       = 'white'; // 'white', 'black', or 'spectator'
let roomPollTimer       = null;
let lobbyWaitingTimer   = null;
let isBoardFlipped      = false;
let lastOnlineMoveCount = -1;

// Piece unicode map
const PIECE_UNICODE = {
  wK: '♔', wQ: '♕', wR: '♖', wB: '♗', wN: '♘', wP: '♙',
  bK: '♚', bQ: '♛', bR: '♜', bB: '♝', bN: '♞', bP: '♟'
};

const PIECE_NAMES = {
  K: 'King', Q: 'Queen', R: 'Rook', B: 'Bishop', N: 'Knight', P: 'Pawn'
};

// ─── API Helper ───────────────────────────────────────────────────────────
async function api(path, method = 'GET', body = null) {
  try {
    const opts = { method, headers: { 'Content-Type': 'application/json' } };
    if (body) opts.body = JSON.stringify(body);
    const res = await fetch(API + path, opts);
    return await res.json();
  } catch (e) {
    console.error('API Error:', path, e);
    return { error: 'Cannot connect to backend (' + API + '). Check server status.' };
  }
}

// ─── Toast Notifications ──────────────────────────────────────────────────
function showToast(msg, type = 'info') {
  const container = document.getElementById('toastContainer');
  if (!container) return;
  const icons = { success: '✓', error: '✕', info: 'ℹ' };
  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;
  toast.innerHTML = `<span>${icons[type] || 'ℹ'}</span> ${msg}`;
  container.appendChild(toast);
  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateX(100%)';
    toast.style.transition = '0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}

// ═══════════════════════════════════════════════════════════════════════════
//  SERVER SETTINGS MODAL
// ═══════════════════════════════════════════════════════════════════════════
function openServerSettings() {
  const input = document.getElementById('customApiUrl');
  if (input) input.value = API;
  document.getElementById('serverSettingsModal').classList.add('active');
}

function closeServerSettings() {
  document.getElementById('serverSettingsModal').classList.remove('active');
}

function saveServerSettings() {
  const val = document.getElementById('customApiUrl').value.trim();
  updateApiUrl(val);
  closeServerSettings();
  showToast(`Backend set to: ${API}`, 'success');
}

function resetServerSettings() {
  localStorage.removeItem('chessverse_api_url');
  API = getApiUrl();
  document.getElementById('customApiUrl').value = API;
  closeServerSettings();
  showToast(`Reset to default: ${API}`, 'info');
}

// ═══════════════════════════════════════════════════════════════════════════
//  PROMOTION MODAL
// ═══════════════════════════════════════════════════════════════════════════
let promotionResolve = null;

function showPromotionModal() {
  return new Promise((resolve) => {
    document.getElementById('promotionModal').classList.add('active');
    promotionResolve = resolve;
  });
}

function selectPromotion(piece) {
  document.getElementById('promotionModal').classList.remove('active');
  if (promotionResolve) {
    promotionResolve(piece);
    promotionResolve = null;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  LOGIN & AUTH
// ═══════════════════════════════════════════════════════════════════════════
function handleLogin(e) {
  e.preventDefault();
  const user = document.getElementById('username').value.trim();
  const pass = document.getElementById('password').value;
  doLogin(user, pass);
  return false;
}

function quickGuestLogin() {
  const randomNum = Math.floor(1000 + Math.random() * 9000);
  const guestUser = `Guest_${randomNum}`;
  currentUser = {
    username: guestUser,
    role: 'player',
    rank: 'Beginner',
    tier: 'pawn',
    exp: 0,
    points: 100,
    streak: 1
  };
  document.getElementById('loginScreen').classList.add('hidden');
  document.getElementById('mainNav').classList.remove('hidden');
  updateNav();
  showPage('play');
  showToast(`Logged in as ${guestUser}`, 'success');

  // If URL has ?room=XXXX, auto open join modal
  checkUrlRoomParameter();
}

async function doLogin(username, password) {
  const data = await api('/login', 'POST', { username, password });
  if (data.error) {
    const errEl = document.getElementById('loginError');
    errEl.textContent = data.error;
    errEl.style.display = 'block';
    SoundEngine.playError();
    return;
  }
  currentUser = data;
  document.getElementById('loginScreen').classList.add('hidden');
  document.getElementById('mainNav').classList.remove('hidden');
  updateNav();
  showPage('play');
  showToast(`Welcome back, ${currentUser.username}!`, 'success');

  // Check URL room invite
  checkUrlRoomParameter();
}

async function doLogout() {
  stopActiveRoomPolling();
  stopLobbyWaiting();
  await api('/logout', 'POST');
  currentUser = null;
  gameActive = false;
  isOnlineRoom = false;
  currentRoomId = null;
  document.getElementById('mainNav').classList.add('hidden');
  hideAllPages();
  document.getElementById('loginScreen').classList.remove('hidden');
  document.getElementById('loginError').style.display = 'none';
  document.getElementById('username').value = '';
  document.getElementById('password').value = '';
}

function updateNav() {
  if (!currentUser) return;
  document.getElementById('navUsername').textContent = currentUser.username;
  document.getElementById('navRank').textContent = currentUser.rank || 'Beginner';
  document.getElementById('navAvatarLetter').textContent = currentUser.username[0].toUpperCase();

  const badge = document.getElementById('navStreak');
  if (currentUser.streak > 0) {
    badge.textContent = currentUser.streak;
    badge.classList.remove('hidden');
  } else {
    badge.classList.add('hidden');
  }

  if (currentUser.role === 'admin') {
    document.getElementById('navAdmin').classList.remove('hidden');
  } else {
    document.getElementById('navAdmin').classList.add('hidden');
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  PAGE NAVIGATION
// ═══════════════════════════════════════════════════════════════════════════
function hideAllPages() {
  ['pagePlay','pageGame','pageStore','pageTrivia','pageProfile','pageAdmin'].forEach(
    id => document.getElementById(id).classList.add('hidden')
  );
  ['navPlay','navStore','navTrivia','navProfile','navAdmin'].forEach(
    id => document.getElementById(id).classList.remove('active')
  );
}

function showPage(page) {
  hideAllPages();
  switch(page) {
    case 'play':
      if (gameActive) {
        document.getElementById('pageGame').classList.remove('hidden');
      } else {
        document.getElementById('pagePlay').classList.remove('hidden');
      }
      document.getElementById('navPlay').classList.add('active');
      break;
    case 'store':
      document.getElementById('pageStore').classList.remove('hidden');
      document.getElementById('navStore').classList.add('active');
      loadStore();
      break;
    case 'trivia':
      document.getElementById('pageTrivia').classList.remove('hidden');
      document.getElementById('navTrivia').classList.add('active');
      loadTrivia();
      break;
    case 'profile':
      document.getElementById('pageProfile').classList.remove('hidden');
      document.getElementById('navProfile').classList.add('active');
      loadProfile();
      break;
    case 'admin':
      document.getElementById('pageAdmin').classList.remove('hidden');
      document.getElementById('navAdmin').classList.add('active');
      break;
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  GAME FLOW (Local & AI)
// ═══════════════════════════════════════════════════════════════════════════
async function startGame(mode) {
  stopActiveRoomPolling();
  stopLobbyWaiting();
  isOnlineRoom = false;
  currentRoomId = null;
  gameMode = mode;
  isBoardFlipped = false;

  document.getElementById('onlineBanner').classList.add('hidden');
  document.getElementById('btnHint').style.display = '';
  document.getElementById('btnUndo').style.display = '';

  const data = await api('/newgame', 'POST', { mode });
  if (data.error) { showToast(data.error, 'error'); return; }

  gameActive = true;
  selectedSquare = null;
  legalMoves = [];
  lastMove = null;
  hintHighlight = null;
  capturedWhite = [];
  capturedBlack = [];
  whiteScore = 0;
  blackScore = 0;
  boardInitialized = false;

  if (mode === 'easy')   document.getElementById('hintCount').textContent = '∞';
  if (mode === 'medium') document.getElementById('hintCount').textContent = '3';
  if (mode === 'hard')   document.getElementById('hintCount').textContent = '0';
  if (mode === 'versus') document.getElementById('hintCount').textContent = '3';

  document.getElementById('whitePlayerLabel').textContent = '⬜ ' + (currentUser ? currentUser.username : 'White');
  document.getElementById('blackPlayerLabel').textContent = mode === 'versus' ? '⬛ Player 2' : '⬛ AI Bot';

  await refreshBoard();
  showPage('play');
  showToast(`Game started — ${mode.toUpperCase()} mode`, 'success');
}

async function refreshBoard() {
  const data = await api('/state');
  if (data.error) return;
  boardData = data.board.board;
  currentTurn = data.board.turn;

  renderBoard();
  updateTurnIndicator();
  updateScores();

  const movesData = await api('/moves');
  if (movesData && movesData.moves !== undefined) {
    updateMoveLog(movesData.moves);
  }

  if (data.board.checkmate) {
    endGame(currentTurn === 'white' ? 'black' : 'white', 'checkmate');
  } else if (data.board.stalemate) {
    endGame('draw', 'stalemate');
  } else if (data.board.inCheck) {
    SoundEngine.playCheck();
    showToast(`${currentTurn.charAt(0).toUpperCase() + currentTurn.slice(1)} is in CHECK!`, 'error');
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  ONLINE MULTIPLAYER (Room Code Matchmaking)
// ═══════════════════════════════════════════════════════════════════════════
function openRoomLobby() {
  if (!currentUser) {
    quickGuestLogin();
  }
  document.getElementById('roomLobbyModal').classList.add('active');
  switchLobbyTab('create');
}

function closeRoomLobby() {
  document.getElementById('roomLobbyModal').classList.remove('active');
  stopLobbyWaiting();
}

function switchLobbyTab(tab) {
  const tabCreate = document.getElementById('tabCreateRoom');
  const tabJoin = document.getElementById('tabJoinRoom');
  const secCreate = document.getElementById('lobbyCreateSection');
  const secJoin = document.getElementById('lobbyJoinSection');

  if (tab === 'create') {
    tabCreate.classList.add('active');
    tabJoin.classList.remove('active');
    secCreate.classList.remove('hidden');
    secJoin.classList.add('hidden');
  } else {
    tabJoin.classList.add('active');
    tabCreate.classList.remove('active');
    secJoin.classList.remove('hidden');
    secCreate.classList.add('hidden');
    document.getElementById('joinRoomCodeInput').focus();
  }
}

async function createOnlineRoom() {
  const colorOptions = document.getElementsByName('colorPref');
  let prefColor = 'white';
  for (const opt of colorOptions) {
    if (opt.checked) prefColor = opt.value;
  }

  const username = currentUser ? currentUser.username : 'Host';
  const data = await api('/room/create', 'POST', { username, preferredColor: prefColor });

  if (data.error) {
    showToast(data.error, 'error');
    return;
  }

  currentRoomId = data.roomId;
  myOnlineColor = data.yourColor;
  isOnlineRoom = true;
  gameMode = 'online';

  // Show Waiting Screen in Modal
  document.getElementById('roomCreationForm').classList.add('hidden');
  document.getElementById('roomWaitingScreen').classList.remove('hidden');
  document.getElementById('createdRoomCode').textContent = currentRoomId;

  // Start polling lobby until opponent joins
  startLobbyWaiting(currentRoomId);
}

function cancelCreatedRoom() {
  stopLobbyWaiting();
  isOnlineRoom = false;
  currentRoomId = null;
  document.getElementById('roomCreationForm').classList.remove('hidden');
  document.getElementById('roomWaitingScreen').classList.add('hidden');
}

function startLobbyWaiting(roomId) {
  stopLobbyWaiting();
  lobbyWaitingTimer = setInterval(async () => {
    const data = await api('/room/state', 'POST', {
      roomId,
      username: currentUser ? currentUser.username : ''
    });

    if (data.status === 'active') {
      // Opponent joined!
      stopLobbyWaiting();
      closeRoomLobby();
      SoundEngine.playPurchase();
      showToast(`Opponent joined! Game starting...`, 'success');
      enterActiveOnlineRoom(data);
    }
  }, 1000);
}

function stopLobbyWaiting() {
  if (lobbyWaitingTimer) {
    clearInterval(lobbyWaitingTimer);
    lobbyWaitingTimer = null;
  }
}

async function joinOnlineRoomFromInput() {
  const input = document.getElementById('joinRoomCodeInput');
  const code = input.value.trim().toUpperCase();
  const errEl = document.getElementById('joinError');
  errEl.style.display = 'none';

  if (!code || code.length < 4) {
    errEl.textContent = 'Please enter a valid 6-character room code.';
    errEl.style.display = 'block';
    return;
  }

  const username = currentUser ? currentUser.username : 'Guest';
  const data = await api('/room/join', 'POST', { roomId: code, username });

  if (data.error) {
    errEl.textContent = data.error;
    errEl.style.display = 'block';
    SoundEngine.playError();
    return;
  }

  currentRoomId = code;
  isOnlineRoom = true;
  gameMode = 'online';
  myOnlineColor = data.yourColor;

  closeRoomLobby();
  SoundEngine.playPurchase();
  showToast(`Joined Room ${code} as ${myOnlineColor.toUpperCase()}`, 'success');
  enterActiveOnlineRoom(data);
}

function enterActiveOnlineRoom(roomData) {
  gameActive = true;
  selectedSquare = null;
  legalMoves = [];
  lastMove = null;
  hintHighlight = null;
  capturedWhite = [];
  capturedBlack = [];
  boardInitialized = false;

  // Auto flip board if playing black
  isBoardFlipped = (myOnlineColor === 'black');

  // Configure UI
  document.getElementById('onlineBanner').classList.remove('hidden');
  document.getElementById('onlineRoomCodeDisplay').innerHTML = `Room: <strong>${roomData.roomId}</strong>`;
  document.getElementById('onlinePlayerRole').innerHTML = `You: <strong>${myOnlineColor === 'white' ? '⬜ White' : '⬛ Black'}</strong> (${currentUser ? currentUser.username : 'You'})`;
  
  const opponentName = myOnlineColor === 'white' ? (roomData.blackPlayer || 'Waiting...') : (roomData.whitePlayer || 'Waiting...');
  document.getElementById('onlineOpponentStatus').innerHTML = `Opponent: <strong>${opponentName}</strong>`;

  document.getElementById('whitePlayerLabel').textContent = '⬜ ' + (roomData.whitePlayer || 'White');
  document.getElementById('blackPlayerLabel').textContent = '⬛ ' + (roomData.blackPlayer || 'Black');

  // In online mode, disable single-player AI hints and local undo
  document.getElementById('btnHint').style.display = 'none';
  document.getElementById('btnUndo').style.display = 'none';

  // Sync state
  applyRoomState(roomData);
  showPage('play');

  // Start fast polling
  startActiveRoomPolling();
}

function startActiveRoomPolling() {
  stopActiveRoomPolling();
  roomPollTimer = setInterval(async () => {
    if (!isOnlineRoom || !currentRoomId || !gameActive) return;
    const data = await api('/room/state', 'POST', {
      roomId: currentRoomId,
      username: currentUser ? currentUser.username : ''
    });
    if (data.error) return;

    applyRoomState(data);
  }, 600);
}

function stopActiveRoomPolling() {
  if (roomPollTimer) {
    clearInterval(roomPollTimer);
    roomPollTimer = null;
  }
}

function applyRoomState(data) {
  if (!data || !data.board) return;

  const prevMoveCount = lastOnlineMoveCount;
  lastOnlineMoveCount = data.moveCount || 0;

  boardData = data.board.board;
  currentTurn = data.turn;
  whiteScore = data.whiteScore || 0;
  blackScore = data.blackScore || 0;

  // Check if opponent made a move
  if (prevMoveCount >= 0 && data.moveCount > prevMoveCount) {
    SoundEngine.playMove();
    if (data.lastMove && data.lastMove.fromRow >= 0) {
      lastMove = data.lastMove;
    }
  }

  // Update online banner opponent name if just joined
  const opponentName = myOnlineColor === 'white' ? (data.blackPlayer || 'Waiting...') : (data.whitePlayer || 'Waiting...');
  document.getElementById('onlineOpponentStatus').innerHTML = `Opponent: <strong>${opponentName}</strong>`;

  renderBoard();
  updateTurnIndicator();
  updateScores();
  updateMoveLog(data.moves);

  // Check game end in online mode
  if (data.status === 'finished') {
    stopActiveRoomPolling();
    endGame(data.winner, data.winReason || 'checkmate');
  } else if (data.inCheck) {
    if (prevMoveCount !== data.moveCount) {
      SoundEngine.playCheck();
      showToast(`${currentTurn.charAt(0).toUpperCase() + currentTurn.slice(1)} is in CHECK!`, 'error');
    }
  }
}

function copyRoomCode() {
  if (!currentRoomId) return;
  navigator.clipboard.writeText(currentRoomId).then(() => {
    showToast(`Room code ${currentRoomId} copied!`, 'success');
  }).catch(() => {
    showToast(`Code: ${currentRoomId}`, 'info');
  });
}

function copyInviteLink() {
  if (!currentRoomId) return;
  const url = `${window.location.origin}${window.location.pathname}?room=${currentRoomId}`;
  navigator.clipboard.writeText(url).then(() => {
    showToast('Match invite link copied to clipboard!', 'success');
  }).catch(() => {
    showToast(`Link: ${url}`, 'info');
  });
}

function checkUrlRoomParameter() {
  const urlParams = new URLSearchParams(window.location.search);
  const roomCode = urlParams.get('room');
  if (roomCode) {
    openRoomLobby();
    switchLobbyTab('join');
    document.getElementById('joinRoomCodeInput').value = roomCode.toUpperCase();
  }
}

async function requestOnlineRematch() {
  if (!isOnlineRoom || !currentRoomId) return;
  const data = await api('/room/rematch', 'POST', {
    roomId: currentRoomId,
    username: currentUser ? currentUser.username : ''
  });
  if (data.error) {
    showToast(data.error, 'error');
    return;
  }
  document.getElementById('winModal').classList.remove('active');
  stopConfetti();
  // Swap our perspective color for rematch
  myOnlineColor = (myOnlineColor === 'white') ? 'black' : 'white';
  isBoardFlipped = (myOnlineColor === 'black');
  enterActiveOnlineRoom(data);
  showToast('Rematch started! Colors swapped.', 'success');
}

// ═══════════════════════════════════════════════════════════════════════════
//  BOARD RENDERING & FLIPPING (Responsive DOM + Perspective Support)
// ═══════════════════════════════════════════════════════════════════════════
let boardInitialized = false;
let boardSquares = [];

function initBoardDOM() {
  const boardEl = document.getElementById('chessBoard');
  boardEl.innerHTML = '';
  boardSquares = [];

  for (let displayRow = 0; displayRow < 8; displayRow++) {
    for (let displayCol = 0; displayCol < 8; displayCol++) {
      const sq = document.createElement('div');
      const isLight = (displayRow + displayCol) % 2 === 0;
      sq.className = `square ${isLight ? 'light' : 'dark'}`;
      sq.dataset.displayRow = displayRow;
      sq.dataset.displayCol = displayCol;

      const pieceEl = document.createElement('div');
      pieceEl.className = 'chess-piece';
      pieceEl.draggable = true;
      pieceEl.style.display = 'none';
      sq.appendChild(pieceEl);

      boardEl.appendChild(sq);
      boardSquares.push(sq);
    }
  }

  // Board click handler (Calculates actual internal board coordinates considering flip)
  boardEl.addEventListener('click', (e) => {
    const sq = e.target.closest('.square');
    if (!sq) return;
    const dispRow = parseInt(sq.dataset.displayRow);
    const dispCol = parseInt(sq.dataset.displayCol);
    const { row, col } = displayToBoardCoords(dispRow, dispCol);
    handleSquareClick(row, col);
  });

  // Board drag-and-drop
  boardEl.addEventListener('dragstart', (e) => {
    const pieceEl = e.target.closest('.chess-piece');
    if (!pieceEl) return;
    const sq = pieceEl.parentElement;
    const dispRow = parseInt(sq.dataset.displayRow);
    const dispCol = parseInt(sq.dataset.displayCol);
    const { row, col } = displayToBoardCoords(dispRow, dispCol);
    e.dataTransfer.setData('text/plain', `${row},${col}`);
    pieceEl.classList.add('dragging');
    handleSquareClick(row, col);
  });

  boardEl.addEventListener('dragend', (e) => {
    const pieceEl = e.target.closest('.chess-piece');
    if (pieceEl) pieceEl.classList.remove('dragging');
  });

  boardEl.addEventListener('dragover', (e) => e.preventDefault());

  boardEl.addEventListener('drop', (e) => {
    e.preventDefault();
    const sq = e.target.closest('.square');
    if (!sq) return;
    const dispRow = parseInt(sq.dataset.displayRow);
    const dispCol = parseInt(sq.dataset.displayCol);
    const { row, col } = displayToBoardCoords(dispRow, dispCol);
    if (selectedSquare) {
      handleSquareClick(row, col);
    }
  });

  boardInitialized = true;
}

// Convert screen display index to internal Board coordinates (0-7, where 0=Rank 1 White back rank)
function displayToBoardCoords(dispRow, dispCol) {
  if (isBoardFlipped) {
    return {
      row: dispRow,           // display top is rank 1 (row 0)
      col: 7 - dispCol        // display left is file h (col 7)
    };
  } else {
    return {
      row: 7 - dispRow,       // display top is rank 8 (row 7)
      col: dispCol            // display left is file a (col 0)
    };
  }
}

// Convert internal Board coordinates to screen display indices
function boardToDisplayCoords(row, col) {
  if (isBoardFlipped) {
    return {
      dispRow: row,
      dispCol: 7 - col
    };
  } else {
    return {
      dispRow: 7 - row,
      dispCol: col
    };
  }
}

function flipBoard() {
  isBoardFlipped = !isBoardFlipped;
  updateBoardLabels();
  renderBoard();
  showToast(isBoardFlipped ? 'Board flipped (Black view)' : 'Board standard (White view)', 'info');
}

function updateBoardLabels() {
  const labelsRow = document.getElementById('boardLabelsRow');
  if (!labelsRow) return;
  const files = isBoardFlipped ? ['h','g','f','e','d','c','b','a'] : ['a','b','c','d','e','f','g','h'];
  labelsRow.innerHTML = files.map(f => `<span>${f}</span>`).join('');
}

function renderBoard() {
  if (!boardData) return;
  if (!boardInitialized) initBoardDOM();

  updateBoardLabels();

  for (let dispRow = 0; dispRow < 8; dispRow++) {
    for (let dispCol = 0; dispCol < 8; dispCol++) {
      const idx = dispRow * 8 + dispCol;
      const sq = boardSquares[idx];
      const { row: boardRow, col: boardCol } = displayToBoardCoords(dispRow, dispCol);

      // boardData in C++ is indexed as boardData[7 - boardRow][boardCol]
      const internalArrayRow = 7 - boardRow;
      const piece = (boardData[internalArrayRow] && boardData[internalArrayRow][boardCol]) || '0';
      const isLight = (dispRow + dispCol) % 2 === 0;

      sq.className = `square ${isLight ? 'light' : 'dark'}`;

      // Highlight last move
      if (lastMove) {
        if (lastMove.fromRow === boardRow && lastMove.fromCol === boardCol) sq.classList.add('last-from');
        if (lastMove.toRow === boardRow && lastMove.toCol === boardCol)     sq.classList.add('last-to');
      }

      // Highlight hint
      if (hintHighlight) {
        if (hintHighlight.fromRow === boardRow && hintHighlight.fromCol === boardCol) sq.classList.add('hint-from');
        if (hintHighlight.toRow === boardRow && hintHighlight.toCol === boardCol)     sq.classList.add('hint-to');
      }

      // Highlight selected
      if (selectedSquare && selectedSquare.row === boardRow && selectedSquare.col === boardCol) {
        sq.classList.add('selected');
      }

      // Legal move markers
      const isLegal = legalMoves.some(m => m.toRow === boardRow && m.toCol === boardCol);
      if (isLegal) {
        if (piece && piece !== '0') sq.classList.add('legal-capture');
        else                        sq.classList.add('legal-move');
      }

      // Render Piece
      const pieceEl = sq.firstElementChild;
      if (piece && piece !== '0') {
        const color = piece[0] === 'w' ? 'white' : 'black';
        pieceEl.className = `chess-piece piece-${color}`;
        pieceEl.textContent = PIECE_UNICODE[piece] || '?';
        pieceEl.draggable = true;
        pieceEl.style.display = '';
      } else {
        pieceEl.style.display = 'none';
        pieceEl.textContent = '';
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  MOVE & CLICK HANDLING
// ═══════════════════════════════════════════════════════════════════════════
async function handleSquareClick(row, col) {
  if (!gameActive) return;
  hintHighlight = null;

  // In online room, enforce player turn
  if (isOnlineRoom) {
    if (currentTurn !== myOnlineColor) {
      showToast(`It is ${currentTurn.toUpperCase()}'s turn. Please wait for your opponent.`, 'info');
      return;
    }
  }

  const internalArrayRow = 7 - row;
  const piece = boardData[internalArrayRow] && boardData[internalArrayRow][col];

  if (selectedSquare) {
    // Attempt move
    const isLegal = legalMoves.some(m => m.toRow === row && m.toCol === col);
    if (isLegal) {
      let promotionChoice = null;
      const fromInternalRow = 7 - selectedSquare.row;
      const selectedPiece = boardData[fromInternalRow][selectedSquare.col];

      if (selectedPiece === 'wP' && row === 7) promotionChoice = await showPromotionModal();
      if (selectedPiece === 'bP' && row === 0) promotionChoice = await showPromotionModal();

      await makeMove(selectedSquare.row, selectedSquare.col, row, col, promotionChoice);
    } else {
      // Clicked on own piece? Re-select
      if (piece && piece !== '0' && isPieceOfTurn(piece)) {
        // Enforce online player cannot select opponent's pieces
        if (isOnlineRoom && piece[0] !== myOnlineColor[0]) {
          selectedSquare = null;
          legalMoves = [];
          renderBoard();
          return;
        }
        await selectSquare(row, col);
        return;
      }
      // Deselect
      selectedSquare = null;
      legalMoves = [];
      renderBoard();
      return;
    }
  } else {
    // First click — select piece
    if (piece && piece !== '0' && isPieceOfTurn(piece)) {
      if (isOnlineRoom && piece[0] !== myOnlineColor[0]) {
        return;
      }
      await selectSquare(row, col);
    }
  }
}

function isPieceOfTurn(piece) {
  if (!piece || piece === '0') return false;
  return (piece[0] === 'w' && currentTurn === 'white') ||
         (piece[0] === 'b' && currentTurn === 'black');
}

async function selectSquare(row, col) {
  selectedSquare = { row, col };
  legalMoves = [];

  let data;
  if (isOnlineRoom && currentRoomId) {
    data = await api('/room/legal_moves', 'POST', {
      roomId: currentRoomId,
      row: String(row),
      col: String(col)
    });
  } else {
    data = await api('/legal_moves', 'POST', { row: String(row), col: String(col) });
  }

  if (data.error) {
    showToast(data.error, 'error');
    return;
  }

  if (selectedSquare && selectedSquare.row === row && selectedSquare.col === col) {
    legalMoves = Array.isArray(data) ? data : [];
    renderBoard();
  }
}

async function makeMove(fromRow, fromCol, toRow, toCol, promotion = null) {
  const internalToRow = 7 - toRow;
  const targetPiece = boardData[internalToRow] && boardData[internalToRow][toCol];
  const isCapture = targetPiece && targetPiece !== '0';

  const body = {
    fromRow: String(fromRow), fromCol: String(fromCol),
    toRow: String(toRow), toCol: String(toCol)
  };
  if (promotion) body.promotion = promotion;

  let data;
  if (isOnlineRoom && currentRoomId) {
    body.roomId = currentRoomId;
    body.username = currentUser ? currentUser.username : '';
    data = await api('/room/move', 'POST', body);
  } else {
    data = await api('/move', 'POST', body);
  }

  if (data.error) {
    SoundEngine.playError();
    showToast(data.error, 'error');
    selectedSquare = null;
    legalMoves = [];
    renderBoard();
    return;
  }

  // Play sound & capture animations
  if (isCapture && targetPiece) {
    if (targetPiece[0] === 'b') capturedWhite.push(PIECE_UNICODE[targetPiece]);
    else                         capturedBlack.push(PIECE_UNICODE[targetPiece]);

    if (targetPiece[1] === 'Q') SoundEngine.playQueenCaptured();
    else                        SoundEngine.playCapture();
  } else {
    SoundEngine.playMove();
  }

  lastMove = { fromRow, fromCol, toRow, toCol };
  selectedSquare = null;
  legalMoves = [];

  if (isOnlineRoom) {
    applyRoomState(data);
  } else {
    whiteScore = data.whiteScore || 0;
    blackScore = data.blackScore || 0;
    updateMoveLog(data.moves);
    boardData = data.board.board;
    currentTurn = data.board.turn;
    renderBoard();
    updateTurnIndicator();
    updateScores();
    updateCaptured();

    if (data.board.checkmate) {
      const winner = data.board.turn === 'white' ? 'black' : 'white';
      endGame(winner, 'checkmate');
      return;
    }
    if (data.board.stalemate) {
      endGame('draw', 'stalemate');
      return;
    }
    if (data.board.inCheck) {
      SoundEngine.playCheck();
      showToast(`${currentTurn.charAt(0).toUpperCase() + currentTurn.slice(1)} is in CHECK!`, 'error');
    }

    if (gameMode !== 'versus') {
      await triggerAIMove();
    }
  }
}

async function triggerAIMove() {
  if (!gameActive) return;
  updateTurnIndicator('AI thinking...');

  await new Promise(r => setTimeout(r, 800));

  const data = await api('/ai_move', 'POST');
  if (!gameActive) return;

  if (data.error) {
    updateTurnIndicator();
    return;
  }

  const move = data.move;
  if (move && move.length >= 4) {
    const fc = move.charCodeAt(0) - 97, fr = parseInt(move[1]) - 1;
    const tc = move.charCodeAt(2) - 97, tr = parseInt(move[3]) - 1;
    lastMove = { fromRow: fr, fromCol: fc, toRow: tr, toCol: tc };
  }

  if (data.board) {
    boardData = data.board.board;
    currentTurn = data.board.turn;
  }

  whiteScore = data.whiteScore || whiteScore;
  blackScore = data.blackScore || blackScore;

  SoundEngine.playMove();
  renderBoard();
  updateTurnIndicator();
  updateScores();
  updateMoveLog(data.moves);

  if (data.board && data.board.checkmate) {
    const winner = data.board.turn === 'white' ? 'black' : 'white';
    endGame(winner, 'checkmate');
  } else if (data.board && data.board.stalemate) {
    endGame('draw', 'stalemate');
  } else if (data.board && data.board.inCheck) {
    SoundEngine.playCheck();
    showToast(`${currentTurn.charAt(0).toUpperCase() + currentTurn.slice(1)} is in CHECK!`, 'error');
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  UI UPDATES
// ═══════════════════════════════════════════════════════════════════════════
function updateTurnIndicator(customText) {
  const dot = document.getElementById('turnDot');
  const text = document.getElementById('turnText');
  dot.className = `turn-dot ${currentTurn}`;
  
  if (customText) {
    text.textContent = customText;
  } else if (isOnlineRoom) {
    const isMyTurn = (currentTurn === myOnlineColor);
    text.textContent = isMyTurn ? `Your Turn (${currentTurn.toUpperCase()})` : `Opponent's Turn...`;
  } else {
    text.textContent = `${currentTurn.charAt(0).toUpperCase() + currentTurn.slice(1)}'s Turn`;
  }
}

function updateScores() {
  document.getElementById('whiteScore').textContent = whiteScore;
  document.getElementById('blackScore').textContent = blackScore;
}

function updateCaptured() {
  document.getElementById('capturedWhite').textContent = capturedWhite.join(' ');
  document.getElementById('capturedBlack').textContent = capturedBlack.join(' ');
}

function updateMoveLog(movesStr) {
  const log = document.getElementById('moveLog');
  if (!movesStr) {
    log.innerHTML = '';
    return;
  }
  const moves = movesStr.split(' ').filter(m => m.length > 0);
  log.innerHTML = moves.map((m, i) => `<div class="move-entry"><span>${i+1}.</span> ${m}</div>`).join('');
  log.scrollTop = log.scrollHeight;
}

// ═══════════════════════════════════════════════════════════════════════════
//  GAME ACTIONS (Hint, Undo, Copy, Analyze, Resign)
// ═══════════════════════════════════════════════════════════════════════════
async function requestHint() {
  if (!gameActive || isOnlineRoom) return;
  const data = await api('/hint', 'POST', {});
  if (data.error) {
    showToast(data.error, 'error');
    SoundEngine.playError();
    return;
  }
  hintHighlight = {
    fromRow: data.fromRow, fromCol: data.fromCol,
    toRow: data.toRow, toCol: data.toCol
  };
  document.getElementById('hintCount').textContent = data.hintsLeft || '0';
  SoundEngine.playHint();
  renderBoard();
  showToast(`Hint: Move ${data.from} → ${data.to}`, 'info');
}

async function undoMove() {
  if (isOnlineRoom) {
    showToast('Undo is not available in online matches.', 'info');
    return;
  }
  const data = await api('/undo', 'POST');
  if (data.status === 'ok') {
    selectedSquare = null;
    legalMoves = [];
    hintHighlight = null;
    lastMove = null;
    await refreshBoard();
    showToast('Undo successful', 'success');
  } else {
    showToast(data.message || 'Cannot undo', 'error');
  }
}

function copyMoves() {
  const log = document.getElementById('moveLog');
  const text = log.innerText;
  navigator.clipboard.writeText(text).then(() => {
    showToast('Move sequence copied to clipboard!', 'success');
  }).catch(() => {
    showToast('Move sequence copied!', 'success');
  });
}

async function analyzeMoves() {
  const seq = document.getElementById('analyzeInput').value.trim();
  if (!seq) { showToast('Paste a move sequence first', 'error'); return; }
  const data = await api('/analyze', 'POST', { sequence: seq, color: currentTurn });
  if (data.error) { showToast(data.error, 'error'); return; }
  document.getElementById('analyzeResult').textContent = `Best counter: ${data.counterMove}`;
  showToast(`Counter move: ${data.counterMove}`, 'success');
}

function resignGame() {
  if (!gameActive) return;
  if (confirm('Are you sure you want to resign this match?')) {
    gameActive = false;
    if (isOnlineRoom && currentRoomId) {
      api('/room/resign', 'POST', {
        roomId: currentRoomId,
        username: currentUser ? currentUser.username : ''
      }).catch(e => console.error(e));
      const winner = myOnlineColor === 'white' ? 'black' : 'white';
      endGame(winner, 'resignation');
    } else {
      api('/resign', 'POST').catch(e => console.error(e));
      const winner = gameMode === 'versus' ? (currentTurn === 'white' ? 'black' : 'white') : 'black';
      endGame(winner, 'resignation');
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  GAME END / WIN MODAL
// ═══════════════════════════════════════════════════════════════════════════
function endGame(winner, reason) {
  gameActive = false;
  const modal = document.getElementById('winModal');
  const rematchBtn = document.getElementById('btnOnlineRematch');

  if (isOnlineRoom) {
    rematchBtn.classList.remove('hidden');
  } else {
    rematchBtn.classList.add('hidden');
  }

  if (winner === 'draw') {
    document.getElementById('winTrophy').textContent = '🤝';
    document.getElementById('winTitle').textContent = 'Stalemate!';
    document.getElementById('winSubtitle').textContent = 'The game ended in a draw.';
    SoundEngine.playDefeat();
  } else {
    let isPlayerWin = false;
    if (isOnlineRoom) {
      isPlayerWin = (winner === myOnlineColor);
    } else {
      isPlayerWin = (gameMode === 'versus') || (winner === 'white');
    }

    document.getElementById('winTrophy').textContent = isPlayerWin ? '🏆' : '💀';
    document.getElementById('winTitle').textContent = isPlayerWin ? 'Victory!' : 'Defeat';
    document.getElementById('winSubtitle').textContent = `${winner.toUpperCase()} won by ${reason}`;

    if (isPlayerWin) {
      SoundEngine.playVictory();
      launchConfetti();
    } else {
      SoundEngine.playDefeat();
    }
  }

  document.getElementById('winPoints').textContent = Math.max(whiteScore, blackScore);
  document.getElementById('winExp').textContent = Math.floor(Math.max(whiteScore, blackScore) / 2);

  modal.classList.add('active');
}

function closeWinModal() {
  document.getElementById('winModal').classList.remove('active');
  stopConfetti();
  showPage('play');
  gameActive = false;
  isOnlineRoom = false;
  currentRoomId = null;
  refreshUserState();
}

async function refreshUserState() {
  const data = await api('/state');
  if (data && data.user && data.user !== 'null') {
    currentUser = typeof data.user === 'string' ? JSON.parse(data.user) : data.user;
    updateNav();
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  CONFETTI
// ═══════════════════════════════════════════════════════════════════════════
let confettiAnimId = null;
let confettiPieces = [];

function launchConfetti() {
  const canvas = document.getElementById('confetti-canvas');
  if (!canvas) return;
  const ctx = canvas.getContext('2d');
  canvas.width = window.innerWidth;
  canvas.height = window.innerHeight;

  confettiPieces = [];
  const colors = ['#7c5cf5','#ffd700','#22c55e','#3b82f6','#ef4444','#f59e0b','#ec4899'];
  for (let i = 0; i < 120; i++) {
    confettiPieces.push({
      x: Math.random() * canvas.width,
      y: Math.random() * canvas.height - canvas.height,
      w: Math.random() * 10 + 5,
      h: Math.random() * 6 + 3,
      color: colors[Math.floor(Math.random() * colors.length)],
      vx: (Math.random() - 0.5) * 4,
      vy: Math.random() * 3 + 2,
      rot: Math.random() * 360,
      rotSpeed: (Math.random() - 0.5) * 10,
      opacity: 1
    });
  }
  animateConfetti(canvas, ctx);
}

function animateConfetti(canvas, ctx) {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  let active = false;
  for (const p of confettiPieces) {
    p.x += p.vx;
    p.y += p.vy;
    p.rot += p.rotSpeed;
    p.vy += 0.05;
    if (p.y > canvas.height + 50) { p.opacity -= 0.02; }
    if (p.opacity <= 0) continue;
    active = true;

    ctx.save();
    ctx.globalAlpha = p.opacity;
    ctx.translate(p.x, p.y);
    ctx.rotate(p.rot * Math.PI / 180);
    ctx.fillStyle = p.color;
    ctx.fillRect(-p.w/2, -p.h/2, p.w, p.h);
    ctx.restore();
  }
  if (active) confettiAnimId = requestAnimationFrame(() => animateConfetti(canvas, ctx));
}

function stopConfetti() {
  if (confettiAnimId) cancelAnimationFrame(confettiAnimId);
  const canvas = document.getElementById('confetti-canvas');
  if (canvas) {
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  STORE & TRIVIA & PROFILE
// ═══════════════════════════════════════════════════════════════════════════
async function loadStore() {
  if (!currentUser) return;
  document.getElementById('storePoints').textContent = currentUser.points || 0;
  const items = await api('/store');
  if (!Array.isArray(items)) return;

  const boardGrid = document.getElementById('boardSkins');
  const pieceGrid = document.getElementById('pieceSkins');
  boardGrid.innerHTML = '';
  pieceGrid.innerHTML = '';

  const skinIcons = {
    classic: '🪵', neon: '💜', marble: '🏛️', galaxy: '🌌', dragon: '🐉',
    default_pieces: '♟', fire_pieces: '🔥', ice_pieces: '❄️', gold_pieces: '✨', shadow_pieces: '🌑'
  };

  items.forEach(item => {
    const card = document.createElement('div');
    card.className = 'store-card glass';
    card.innerHTML = `
      <div class="store-preview">${skinIcons[item.id] || '🎨'}</div>
      <div class="store-name">${item.name}</div>
      <div class="store-desc">${item.desc}</div>
      ${item.owned ?
        `<div class="store-owned">✓ Equipped / Owned</div>
         <button class="btn btn-secondary btn-sm" style="margin-top:8px;" onclick="equipItem('${item.id}')">Equip</button>` :
        `<div class="store-price">${item.cost} pts</div>
         <button class="btn btn-gold btn-sm" style="margin-top:8px;" onclick="purchaseItem('${item.id}')">Buy</button>`
      }
    `;
    if (item.type === 'board') boardGrid.appendChild(card);
    else                       pieceGrid.appendChild(card);
  });
}

async function purchaseItem(itemId) {
  const data = await api('/purchase', 'POST', { itemId });
  if (data.result === 0) {
    SoundEngine.playPurchase();
    showToast(data.message, 'success');
    await refreshUserState();
    loadStore();
  } else {
    SoundEngine.playError();
    showToast(data.message, 'error');
  }
}

async function equipItem(itemId) {
  const data = await api('/equip', 'POST', { itemId });
  if (data.status === 'ok') {
    showToast('Skin equipped!', 'success');
    loadStore();
  } else {
    showToast('Failed to equip', 'error');
  }
}

async function loadTrivia() {
  const container = document.getElementById('triviaContent');
  const data = await api('/trivia');

  if (data.error) {
    container.innerHTML = `<p style="color:var(--text-secondary); text-align:center;">${data.error}</p>`;
    return;
  }

  if (data.canPlay === false) {
    container.innerHTML = `
      <div style="text-align:center; padding:40px 0;">
        <div style="font-size:3rem; margin-bottom:16px;">✅</div>
        <div style="font-size:1.1rem; font-weight:600; margin-bottom:8px;">Already Played Today!</div>
        <div style="color:var(--text-secondary);">${data.message || 'Come back tomorrow for a new question.'}</div>
      </div>`;
    return;
  }

  const q = data.question;
  if (!q) {
    container.innerHTML = `<p style="color:var(--text-secondary);">No questions available.</p>`;
    return;
  }

  container.innerHTML = `
    <div class="trivia-bonus">🎁 +${q.bonus} bonus points</div>
    <div class="trivia-question">${q.question}</div>
    <div class="trivia-options">
      ${q.options.map((opt, i) => `
        <button class="trivia-option" id="triviaOpt${i}" onclick="submitTrivia(${i})">
          <strong>${String.fromCharCode(65+i)}.</strong> ${opt}
        </button>
      `).join('')}
    </div>
  `;
}

async function submitTrivia(answer) {
  document.querySelectorAll('.trivia-option').forEach(el => {
    el.style.pointerEvents = 'none';
  });

  const data = await api('/trivia/answer', 'POST', { answer: String(answer) });

  if (data.correct) {
    document.getElementById(`triviaOpt${answer}`).classList.add('correct');
    SoundEngine.playPurchase();
    showToast(`Correct! +${data.bonus} points!`, 'success');
  } else {
    document.getElementById(`triviaOpt${answer}`).classList.add('wrong');
    SoundEngine.playError();
    showToast('Wrong answer! Try again tomorrow.', 'error');
  }

  if (currentUser) {
    currentUser.points = data.newPoints;
    updateNav();
  }
}

async function loadProfile() {
  await refreshUserState();
  if (!currentUser) return;

  document.getElementById('profileAvatarLetter').textContent = currentUser.username[0].toUpperCase();
  document.getElementById('profileName').textContent = currentUser.username;
  document.getElementById('profileRankTitle').textContent =
    `${getRankEmoji(currentUser.tier)} ${currentUser.rank || 'Beginner'}`;

  document.getElementById('statPoints').textContent = currentUser.points || 0;
  document.getElementById('statExp').textContent = currentUser.exp || 0;
  document.getElementById('statStreak').textContent = currentUser.streak || 0;
  document.getElementById('statRank').textContent = currentUser.rank || 'Beginner';

  const badge = document.getElementById('profileStreak');
  if (currentUser.streak > 0) {
    badge.textContent = currentUser.streak;
    badge.classList.remove('hidden');
  }

  updateExpBar(currentUser.exp || 0);

  const lb = await api('/leaderboard');
  if (Array.isArray(lb)) {
    const tbody = document.getElementById('leaderboardBody');
    tbody.innerHTML = lb.map((u, i) => `
      <tr>
        <td class="lb-rank ${i < 3 ? 'lb-rank-' + (i+1) : ''}">${i === 0 ? '🥇' : i === 1 ? '🥈' : i === 2 ? '🥉' : i+1}</td>
        <td style="font-weight:600;">${u.username}</td>
        <td>${getRankEmoji(u.tier)} ${u.title}</td>
        <td style="font-family:var(--font-display);">${u.exp}</td>
        <td style="color:var(--gold);">${u.points}</td>
        <td>${u.streak > 0 ? '🔥' + u.streak : '—'}</td>
      </tr>
    `).join('');
  }
}

function getRankEmoji(tier) {
  const emojis = { pawn:'🪨', knight:'⚔️', rook:'🏰', bishop:'👑', queen:'🔱', king:'♾️' };
  return emojis[tier] || '🪨';
}

function updateExpBar(exp) {
  const ranks = [
    { title:'Beginner', min:0 },
    { title:'Apprentice', min:500 },
    { title:'Tactician', min:1500 },
    { title:'Strategist', min:3000 },
    { title:'Grandmaster', min:6000 },
    { title:'Legend', min:10000 }
  ];
  let current = ranks[0], next = ranks[1];
  for (let i = 0; i < ranks.length - 1; i++) {
    if (exp >= ranks[i].min) { current = ranks[i]; next = ranks[i+1]; }
  }
  if (exp >= ranks[ranks.length-1].min) {
    current = ranks[ranks.length-1];
    next = { title: 'MAX', min: current.min + 1 };
  }

  const progress = Math.min(100, ((exp - current.min) / (next.min - current.min)) * 100);
  document.getElementById('expLabel').textContent = `${exp} / ${next.min} EXP`;
  document.getElementById('nextRankLabel').textContent =
    next.title === 'MAX' ? 'Max Rank!' : `Next: ${next.title}`;
  document.getElementById('expFill').style.width = `${progress}%`;
}

// ═══════════════════════════════════════════════════════════════════════════
//  ADMIN
// ═══════════════════════════════════════════════════════════════════════════
async function adminAddUser() {
  const username = document.getElementById('adminNewUser').value.trim();
  const password = document.getElementById('adminNewPass').value;
  const role     = document.getElementById('adminNewRole').value;
  if (!username || !password) { showToast('Fill all fields', 'error'); return; }

  const data = await api('/admin/adduser', 'POST', { username, password, role });
  if (data.status === 'ok') {
    showToast(`User "${username}" created!`, 'success');
    document.getElementById('adminNewUser').value = '';
    document.getElementById('adminNewPass').value = '';
  } else {
    showToast(data.error || 'Failed to create user', 'error');
  }
}

async function adminAddTrivia() {
  const q = document.getElementById('adminTriviaQ').value.trim();
  const opts = [
    document.getElementById('adminOptA').value.trim(),
    document.getElementById('adminOptB').value.trim(),
    document.getElementById('adminOptC').value.trim(),
    document.getElementById('adminOptD').value.trim()
  ];
  const correct = parseInt(document.getElementById('adminTriviaCorrect').value);
  if (!q || opts.some(o => !o)) { showToast('Fill all fields', 'error'); return; }

  showToast('Trivia question added!', 'success');
  document.getElementById('adminTriviaQ').value = '';
  document.getElementById('adminOptA').value = '';
  document.getElementById('adminOptB').value = '';
  document.getElementById('adminOptC').value = '';
  document.getElementById('adminOptD').value = '';
}

// ═══════════════════════════════════════════════════════════════════════════
//  KEYBOARD SHORTCUTS & INIT
// ═══════════════════════════════════════════════════════════════════════════
document.addEventListener('keydown', (e) => {
  if (e.key === 'h' && gameActive && !isOnlineRoom) requestHint();
  if (e.key === 'f' && gameActive) flipBoard();
  if (e.key === 'Escape') {
    selectedSquare = null;
    legalMoves = [];
    hintHighlight = null;
    if (gameActive) renderBoard();
  }
});

// Warm up Audio Context
document.addEventListener('click', () => {
  try { SoundEngine.playMove; } catch(e) {}
}, { once: true });

// Check URL query parameters on initial page load
window.addEventListener('DOMContentLoaded', () => {
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.has('room')) {
    const code = urlParams.get('room');
    if (code) {
      if (!currentUser) {
        quickGuestLogin();
      }
      openRoomLobby();
      switchLobbyTab('join');
      document.getElementById('joinRoomCodeInput').value = code.toUpperCase();
    }
  }
});
