// ═══════════════════════════════════════════════════════════════════════════
// ChessVerse — Sound System
// Web Audio API based sound effects (no external files needed)
// ═══════════════════════════════════════════════════════════════════════════

const SoundEngine = (() => {
  let audioCtx = null;

  function getCtx() {
    if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    return audioCtx;
  }

  function resumeCtx() {
    const ctx = getCtx();
    if (ctx.state === 'suspended') ctx.resume();
  }

  // ── Piece Move: "tok tok" ───────────────────────────────────────────────
  function playMove() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    // First "tok"
    playTok(ctx, now);
    // Second "tok" (slightly delayed)
    playTok(ctx, now + 0.08);
  }

  function playTok(ctx, time) {
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.type = 'square';
    osc.frequency.setValueAtTime(800, time);
    osc.frequency.exponentialRampToValueAtTime(200, time + 0.05);
    gain.gain.setValueAtTime(0.3, time);
    gain.gain.exponentialRampToValueAtTime(0.001, time + 0.06);
    osc.start(time);
    osc.stop(time + 0.06);
  }

  // ── Piece Capture: sharp impact ─────────────────────────────────────────
  function playCapture() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.type = 'sawtooth';
    osc.frequency.setValueAtTime(600, now);
    osc.frequency.exponentialRampToValueAtTime(100, now + 0.15);
    gain.gain.setValueAtTime(0.4, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.15);
    osc.start(now);
    osc.stop(now + 0.15);

    // Noise burst for impact
    const bufferSize = ctx.sampleRate * 0.05;
    const buffer = ctx.createBuffer(1, bufferSize, ctx.sampleRate);
    const data = buffer.getChannelData(0);
    for (let i = 0; i < bufferSize; i++) data[i] = (Math.random() * 2 - 1) * 0.2;
    const noise = ctx.createBufferSource();
    const noiseGain = ctx.createGain();
    noise.buffer = buffer;
    noise.connect(noiseGain);
    noiseGain.connect(ctx.destination);
    noiseGain.gain.setValueAtTime(0.3, now);
    noiseGain.gain.exponentialRampToValueAtTime(0.001, now + 0.05);
    noise.start(now);
  }

  // ── Queen Captured: "FAHHHH" — dramatic falling sound ──────────────────
  function playQueenCaptured() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    // Dramatic descending tone
    const osc1 = ctx.createOscillator();
    const gain1 = ctx.createGain();
    osc1.connect(gain1);
    gain1.connect(ctx.destination);
    osc1.type = 'sawtooth';
    osc1.frequency.setValueAtTime(800, now);
    osc1.frequency.exponentialRampToValueAtTime(80, now + 0.8);
    gain1.gain.setValueAtTime(0.35, now);
    gain1.gain.linearRampToValueAtTime(0.3, now + 0.3);
    gain1.gain.exponentialRampToValueAtTime(0.001, now + 0.8);
    osc1.start(now);
    osc1.stop(now + 0.8);

    // Rumble undertone
    const osc2 = ctx.createOscillator();
    const gain2 = ctx.createGain();
    osc2.connect(gain2);
    gain2.connect(ctx.destination);
    osc2.type = 'sine';
    osc2.frequency.setValueAtTime(100, now);
    osc2.frequency.exponentialRampToValueAtTime(40, now + 0.6);
    gain2.gain.setValueAtTime(0.25, now + 0.1);
    gain2.gain.exponentialRampToValueAtTime(0.001, now + 0.7);
    osc2.start(now + 0.05);
    osc2.stop(now + 0.7);

    // "Ahhh" vocal-like formant
    const osc3 = ctx.createOscillator();
    const gain3 = ctx.createGain();
    const filter = ctx.createBiquadFilter();
    osc3.connect(filter);
    filter.connect(gain3);
    gain3.connect(ctx.destination);
    osc3.type = 'sawtooth';
    osc3.frequency.setValueAtTime(250, now);
    osc3.frequency.exponentialRampToValueAtTime(100, now + 0.7);
    filter.type = 'bandpass';
    filter.frequency.setValueAtTime(700, now);
    filter.frequency.linearRampToValueAtTime(300, now + 0.7);
    filter.Q.value = 5;
    gain3.gain.setValueAtTime(0.2, now + 0.05);
    gain3.gain.exponentialRampToValueAtTime(0.001, now + 0.7);
    osc3.start(now + 0.03);
    osc3.stop(now + 0.7);
  }

  // ── Check Sound ─────────────────────────────────────────────────────────
  function playCheck() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    for (let i = 0; i < 2; i++) {
      const t = now + i * 0.12;
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.type = 'triangle';
      osc.frequency.setValueAtTime(1200, t);
      osc.frequency.exponentialRampToValueAtTime(800, t + 0.08);
      gain.gain.setValueAtTime(0.25, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.1);
      osc.start(t);
      osc.stop(t + 0.1);
    }
  }

  // ── Checkmate / Victory Sound ───────────────────────────────────────────
  function playVictory() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    const notes = [523, 659, 784, 1047]; // C5, E5, G5, C6
    notes.forEach((freq, i) => {
      const t = now + i * 0.15;
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.type = 'triangle';
      osc.frequency.setValueAtTime(freq, t);
      gain.gain.setValueAtTime(0.3, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.4);
      osc.start(t);
      osc.stop(t + 0.4);
    });

    // Final chord
    const chord = [1047, 1318, 1568];
    chord.forEach(freq => {
      const t = now + 0.6;
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.type = 'sine';
      osc.frequency.setValueAtTime(freq, t);
      gain.gain.setValueAtTime(0.15, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.8);
      osc.start(t);
      osc.stop(t + 0.8);
    });
  }

  // ── Defeat Sound ────────────────────────────────────────────────────────
  function playDefeat() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    const notes = [392, 349, 311, 261]; // G4, F4, Eb4, C4 (descending)
    notes.forEach((freq, i) => {
      const t = now + i * 0.2;
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.type = 'sine';
      osc.frequency.setValueAtTime(freq, t);
      gain.gain.setValueAtTime(0.25, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.35);
      osc.start(t);
      osc.stop(t + 0.35);
    });
  }

  // ── Hint Sound (gentle chime) ───────────────────────────────────────────
  function playHint() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    [880, 1100].forEach((f, i) => {
      const t = now + i * 0.08;
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.type = 'sine';
      osc.frequency.setValueAtTime(f, t);
      gain.gain.setValueAtTime(0.15, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.2);
      osc.start(t);
      osc.stop(t + 0.2);
    });
  }

  // ── Error / Invalid Move ────────────────────────────────────────────────
  function playError() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.type = 'square';
    osc.frequency.setValueAtTime(150, now);
    gain.gain.setValueAtTime(0.2, now);
    gain.gain.exponentialRampToValueAtTime(0.001, now + 0.15);
    osc.start(now);
    osc.stop(now + 0.15);
  }

  // ── Purchase Cha-Ching ──────────────────────────────────────────────────
  function playPurchase() {
    resumeCtx();
    const ctx = getCtx();
    const now = ctx.currentTime;

    [1319, 1568, 2093].forEach((f, i) => {
      const t = now + i * 0.06;
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.type = 'triangle';
      osc.frequency.setValueAtTime(f, t);
      gain.gain.setValueAtTime(0.2, t);
      gain.gain.exponentialRampToValueAtTime(0.001, t + 0.15);
      osc.start(t);
      osc.stop(t + 0.15);
    });
  }

  return {
    playMove, playCapture, playQueenCaptured, playCheck,
    playVictory, playDefeat, playHint, playError, playPurchase
  };
})();
