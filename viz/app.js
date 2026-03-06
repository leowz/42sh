/* ─── Token type → colour mapping ──────────────────────────────── */
const TOKEN_COLORS = {
  TOK_WORD:         'var(--c-word)',
  TOK_PIPE:         'var(--c-pipe)',
  TOK_AND:          'var(--c-and)',
  TOK_OR:           'var(--c-or)',
  TOK_SEMICOLON:    'var(--c-semicolon)',
  TOK_AMPERSAND:    'var(--c-ampersand)',
  TOK_NEWLINE:      'var(--c-newline)',
  TOK_REDIR_IN:     'var(--c-redir-in)',
  TOK_REDIR_OUT:    'var(--c-redir-out)',
  TOK_REDIR_APPEND: 'var(--c-redir-app)',
  TOK_HEREDOC:      'var(--c-heredoc)',
  TOK_REDIR_DUP_IN: 'var(--c-redir-dup)',
  TOK_REDIR_DUP_OUT:'var(--c-redir-dup)',
  TOK_LPAREN:       'var(--c-paren)',
  TOK_RPAREN:       'var(--c-paren)',
  TOK_EOF:          'var(--c-eof)',
  TOK_ERROR:        'var(--c-error)',
};

function colorFor(type) {
  return TOKEN_COLORS[type] || '#8b949e';
}

/* ─── State ──────────────────────────────────────────────────────── */
const sessions = [];   // { name, data }
let active = null;     // currently displayed session index
let selectedIdx = null;
let showRelations = true;

/* ─── DOM refs ───────────────────────────────────────────────────── */
const sessionList  = document.getElementById('session-list');
const welcome      = document.getElementById('welcome');
const vizContainer = document.getElementById('viz-container');
const inputDisplay = document.getElementById('input-display');
const tokenStrip   = document.getElementById('token-strip');
const legendEl     = document.getElementById('legend');
const arcSvg       = document.getElementById('arc-diagram');
const detailPanel  = document.getElementById('detail-panel');
const detailType   = document.getElementById('detail-type');
const detailTable  = document.getElementById('detail-table');
const filePicker   = document.getElementById('file-picker');

document.getElementById('detail-close').addEventListener('click', () => {
  detailPanel.style.display = 'none';
  selectedIdx = null;
  document.querySelectorAll('.token-chip').forEach(c => c.classList.remove('selected'));
});

document.getElementById('toggle-relations').addEventListener('click', function () {
  showRelations = !showRelations;
  this.classList.toggle('active', showRelations);
  d3.selectAll('.arc-same-type').style('display', showRelations ? null : 'none');
});

/* ─── Load manifest from ../tokenizations/ ───────────────────────── */
async function loadManifest() {
  const root = getDataRoot();
  try {
    const res = await fetch(`${root}/manifest.json`);
    if (!res.ok) return;
    const files = await res.json();
    for (const fname of files) {
      await loadJsonFile(`${root}/${fname}`, fname);
    }
  } catch (_) {
    // No manifest or CORS in local file:// — user must load manually
  }
}

async function loadJsonFile(url, label) {
  try {
    const res  = await fetch(url);
    const data = await res.json();
    addSession(label, data);
  } catch (e) {
    console.warn('Could not load', url, e);
  }
}

/* ─── File picker (manual load) ─────────────────────────────────── */
filePicker.addEventListener('change', async () => {
  for (const file of filePicker.files) {
    const text = await file.text();
    try {
      addSession(file.name, JSON.parse(text));
    } catch (e) {
      alert(`Could not parse ${file.name}: ${e.message}`);
    }
  }
  filePicker.value = '';
});

/* ─── Session management ─────────────────────────────────────────── */
function addSession(name, data) {
  // Deduplicate by name
  if (sessions.find(s => s.name === name)) return;
  sessions.push({ name, data });
  renderSidebar();
  if (sessions.length === 1) showSession(0);
}

function renderSidebar() {
  sessionList.innerHTML = '';
  if (sessions.length === 0) {
    sessionList.innerHTML = '<p class="hint">No sessions loaded.</p>';
    return;
  }
  sessions.forEach((s, i) => {
    const el = document.createElement('div');
    el.className = 'session-item' + (i === active ? ' active' : '');
    el.title = s.data.input || s.name;
    // Show truncated input if available, else filename
    el.textContent = s.data.input ? s.data.input.slice(0, 34) + (s.data.input.length > 34 ? '…' : '') : s.name;
    el.addEventListener('click', () => showSession(i));
    sessionList.appendChild(el);
  });
}

/* ─── Main render ────────────────────────────────────────────────── */
function showSession(idx) {
  active = idx;
  selectedIdx = null;
  renderSidebar();

  const { data } = sessions[idx];
  welcome.style.display = 'none';
  vizContainer.style.display = 'flex';
  detailPanel.style.display = 'none';

  inputDisplay.textContent = data.input || '';
  renderTokenStrip(data.tokens);
  renderLegend(data.tokens);
  renderArc(data.tokens);
}

/* ─── Token strip ────────────────────────────────────────────────── */
function renderTokenStrip(tokens) {
  tokenStrip.innerHTML = '';
  tokens.forEach((tok, i) => {
    const chip = document.createElement('div');
    chip.className = 'token-chip';
    chip.dataset.idx = i;
    const bg = colorFor(tok.type);
    chip.style.background = hexOpacity(bg, 0.18);
    chip.style.color = bg;

    const val = document.createElement('span');
    val.className = 'chip-value';
    val.textContent = tok.value !== '' ? tok.value : '∅';

    const type = document.createElement('span');
    type.className = 'chip-type';
    type.textContent = tok.type.replace('TOK_', '');

    chip.appendChild(val);
    chip.appendChild(type);
    chip.addEventListener('click', () => selectToken(i, tok));
    tokenStrip.appendChild(chip);
  });
}

function selectToken(idx, tok) {
  selectedIdx = idx;
  document.querySelectorAll('.token-chip').forEach((c, i) => {
    c.classList.toggle('selected', i === idx);
  });
  showDetail(tok);
  highlightArcs(idx);
}

function showDetail(tok) {
  const color = colorFor(tok.type);
  detailType.textContent = tok.type;
  detailType.style.color = color;
  detailTable.innerHTML = `
    <tr><td>value</td><td><code>${escHtml(tok.value)}</code></td></tr>
    <tr><td>io_number</td><td>${tok.io_number === -1 ? '<span style="color:var(--muted)">—</span>' : tok.io_number}</td></tr>
    <tr><td>index</td><td>${selectedIdx}</td></tr>
  `;
  detailPanel.style.display = 'block';
}

/* ─── Legend ─────────────────────────────────────────────────────── */
function renderLegend(tokens) {
  const seen = [...new Set(tokens.map(t => t.type))];
  legendEl.innerHTML = '';
  seen.forEach(type => {
    const item = document.createElement('div');
    item.className = 'legend-item';
    item.innerHTML = `<div class="legend-swatch" style="background:${colorFor(type)}"></div>${type.replace('TOK_', '')}`;
    item.addEventListener('click', () => highlightByType(type));
    legendEl.appendChild(item);
  });
}

/* ─── D3 arc diagram ─────────────────────────────────────────────── */
function renderArc(tokens) {
  const svg = d3.select('#arc-diagram');
  svg.selectAll('*').remove();

  const W = arcSvg.clientWidth || 800;
  const H = 130;
  svg.attr('viewBox', `0 0 ${W} ${H}`);

  const n = tokens.length;
  if (n < 2) return;

  const margin = 40;
  const xScale = d3.scalePoint()
    .domain(d3.range(n))
    .range([margin, W - margin]);

  const yBase = H - 18;

  // Draw arcs between adjacent tokens (i → i+1)
  const arcs = [];
  for (let i = 0; i < n - 1; i++) {
    arcs.push({ from: i, to: i + 1 });
  }
  // Also connect same-type tokens for fun
  for (let i = 0; i < n; i++) {
    for (let j = i + 2; j < n; j++) {
      if (tokens[i].type === tokens[j].type && tokens[i].type !== 'TOK_EOF') {
        arcs.push({ from: i, to: j, sameType: true });
      }
    }
  }

  // Arcs
  const g = svg.append('g');
  g.selectAll('.arc-path')
    .data(arcs)
    .enter()
    .append('path')
    .attr('class', d => `arc-path arc-from-${d.from} arc-to-${d.to}${d.sameType ? ' arc-same-type' : ''}`)
    .attr('d', d => {
      const x1 = xScale(d.from), x2 = xScale(d.to);
      const mx = (x1 + x2) / 2;
      const span = Math.abs(x2 - x1);
      const ry  = -(d.sameType ? span * 0.55 : span * 0.38);
      return `M ${x1} ${yBase} C ${x1} ${yBase - ry}, ${x2} ${yBase - ry}, ${x2} ${yBase}`;
    })
    .attr('stroke', d => d.sameType
      ? colorFor(tokens[d.from].type)
      : '#30363d')
    .attr('stroke-dasharray', d => d.sameType ? '4 3' : null)
    .attr('opacity', d => d.sameType ? 0.3 : 0.6)
    .style('display', d => (d.sameType && !showRelations) ? 'none' : null);

  // Nodes
  g.selectAll('.arc-node')
    .data(tokens)
    .enter()
    .append('circle')
    .attr('class', 'arc-node')
    .attr('cx', (_, i) => xScale(i))
    .attr('cy', yBase)
    .attr('r', 7)
    .attr('fill', d => colorFor(d.type))
    .attr('opacity', 0.85)
    .on('click', (_, d) => {
      const idx = tokens.indexOf(d);
      selectToken(idx, d);
    })
    .append('title').text(d => `${d.type}\n"${d.value}"`);

  // Labels
  g.selectAll('.arc-label')
    .data(tokens)
    .enter()
    .append('text')
    .attr('class', 'arc-label')
    .attr('x', (_, i) => xScale(i))
    .attr('y', yBase - 15)
    .text(d => d.value.length > 6 ? d.value.slice(0, 5) + '…' : d.value || '∅');
}

function highlightArcs(idx) {
  d3.selectAll('.arc-path').classed('hi', false);
  d3.selectAll(`.arc-from-${idx}, .arc-to-${idx}`).classed('hi', true);
}

function highlightByType(type) {
  document.querySelectorAll('.token-chip').forEach((chip, i) => {
    const tok = sessions[active].data.tokens[i];
    chip.style.opacity = tok.type === type ? '1' : '0.25';
  });
  setTimeout(() => {
    document.querySelectorAll('.token-chip').forEach(c => c.style.opacity = '');
  }, 1800);
}

/* ─── Utilities ──────────────────────────────────────────────────── */
function escHtml(s) {
  return (s || '').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

/**
 * Convert a CSS variable or hex/rgb colour to a semi-transparent rgba.
 * For CSS variables we just return the variable with extra opacity via a
 * colour-mix fallback — simpler: return a data-uri colour overlay.
 * We use a transparent background trick with currentColor instead.
 */
function hexOpacity(cssColor, alpha) {
  // We'll use a box-shadow trick; return as-is and handle in CSS via opacity.
  // For simplicity, just return the color string and handle opacity via element.
  return `color-mix(in srgb, ${cssColor} ${Math.round(alpha * 100)}%, transparent)`;
}

/* ─── Configuration ──────────────────────────────────────────────── */
// When served from GitHub Pages with viz/ as root, tokenizations/ sits one
// level up at the repo root.  You can override this via ?data=PATH in the URL.
function getDataRoot() {
  const params = new URLSearchParams(window.location.search);
  return params.get('data') || 'tokenizations';
}

/* ─── Boot ───────────────────────────────────────────────────────── */
loadManifest();
