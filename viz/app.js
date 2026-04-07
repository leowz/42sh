/* ═══════════════════════════════════════════════════════════════════
   42sh Visualizer — Tokenization + AST explorer
   ═══════════════════════════════════════════════════════════════════ */

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

/* ─── AST node type → colour mapping ──────────────────────────── */
const NODE_COLORS = {
  NODE_COMMAND:    '#3fb950',
  NODE_PIPE:       '#ff7b72',
  NODE_AND:        '#ffa657',
  NODE_OR:         '#d2a8ff',
  NODE_SEQUENCE:   '#79c0ff',
  NODE_SUBSHELL:   '#f778ba',
  NODE_BLOCK:      '#56d364',
  NODE_BACKGROUND: '#ffd866',
};

/* Short display labels for node types */
const NODE_LABELS = {
  NODE_COMMAND:    'CMD',
  NODE_PIPE:       'PIPE',
  NODE_AND:        'AND',
  NODE_OR:         'OR',
  NODE_SEQUENCE:   'SEQ',
  NODE_SUBSHELL:   'SUBSH',
  NODE_BLOCK:      'BLOCK',
  NODE_BACKGROUND: 'BG',
};

/* Redir type display strings */
const REDIR_SYMBOLS = {
  '<': '<', '>': '>', '>>': '>>', '<<': '<<', '<&': '<&', '>&': '>&',
};

function tokenColor(type) {
  return TOKEN_COLORS[type] || '#8b949e';
}

function nodeColor(type) {
  return NODE_COLORS[type] || '#8b949e';
}

/* ─── State ──────────────────────────────────────────────────────── */
const sessions = [];     // { input, tokens, tree, tokName, astName }
let active = null;
let selectedIdx = null;
let showRelations = true;

/* ─── Settings (persisted in localStorage) ───────────────────────── */
const SETTINGS_KEY = '42sh-viz-settings';

const defaultSettings = {
  curveStyle: 'cubic',
  nodeFill: 'translucent',
  orientation: 'vertical',
};

function loadSettings() {
  try {
    const raw = localStorage.getItem(SETTINGS_KEY);
    if (raw) return { ...defaultSettings, ...JSON.parse(raw) };
  } catch (_) {}
  return { ...defaultSettings };
}

function saveSettings() {
  localStorage.setItem(SETTINGS_KEY, JSON.stringify(settings));
}

const settings = loadSettings();

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
const astDetail    = document.getElementById('ast-detail');
const astDetailType = document.getElementById('ast-detail-type');
const astDetailBody = document.getElementById('ast-detail-body');

/* ─── Tabs (with localStorage persistence) ───────────────────────── */
function activateTab(tabName) {
  document.querySelectorAll('#tab-bar .tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));
  const tab = document.querySelector(`#tab-bar .tab[data-tab="${tabName}"]`);
  if (tab && !tab.classList.contains('disabled')) {
    tab.classList.add('active');
    document.getElementById(tabName + '-pane').classList.add('active');
    localStorage.setItem('42sh-viz-tab', tabName);
  }
}

function getPreferredTab() {
  return localStorage.getItem('42sh-viz-tab') || 'tokens';
}

document.querySelectorAll('#tab-bar .tab').forEach(tab => {
  tab.addEventListener('click', () => {
    if (tab.classList.contains('disabled')) return;
    activateTab(tab.dataset.tab);
  });
});

/* ─── Token detail close ─────────────────────────────────────────── */
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

/* ─── AST detail close ───────────────────────────────────────────── */
document.getElementById('ast-detail-close').addEventListener('click', () => {
  astDetail.style.display = 'none';
  d3.selectAll('.ast-node').classed('selected', false);
});

/* ─── AST toolbar ────────────────────────────────────────────────── */
document.getElementById('ast-reset-zoom').addEventListener('click', () => {
  if (currentZoom && currentSvg) {
    const svgEl = currentSvg.node();
    const gEl = svgEl.querySelector('g');
    if (!gEl) return;
    const bbox = gEl.getBBox();
    if (!bbox.width || !bbox.height) return;
    const W = svgEl.clientWidth || 800;
    const H = svgEl.clientHeight || 500;
    const pad = 40;
    const scaleX = (W - pad * 2) / bbox.width;
    const scaleY = (H - pad * 2) / bbox.height;
    const s = Math.min(scaleX, scaleY, 1.2);
    const tx = (W - bbox.width * s) / 2 - bbox.x * s;
    const ty = (H - bbox.height * s) / 2 - bbox.y * s;
    currentSvg.transition().duration(500).call(currentZoom.transform, d3.zoomIdentity.translate(tx, ty).scale(s));
  }
});


/* ─── Settings modal ─────────────────────────────────────────────── */
const settingsOverlay = document.getElementById('settings-overlay');
document.getElementById('settings-btn').addEventListener('click', () => {
  settingsOverlay.hidden = false;
  syncSettingsUI();
  drawCurvePreview();
});
document.getElementById('settings-close').addEventListener('click', () => {
  settingsOverlay.hidden = true;
});
settingsOverlay.addEventListener('click', (e) => {
  if (e.target === settingsOverlay) settingsOverlay.hidden = true;
});

function syncSettingsUI() {
  document.querySelectorAll('#curve-options .setting-chip').forEach(b => {
    b.classList.toggle('active', b.dataset.curve === settings.curveStyle);
  });
  document.querySelectorAll('#fill-options .setting-chip').forEach(b => {
    b.classList.toggle('active', b.dataset.fill === settings.nodeFill);
  });
  document.querySelectorAll('#orientation-options .setting-chip').forEach(b => {
    b.classList.toggle('active', b.dataset.orientation === settings.orientation);
  });
}

/* Curve style buttons */
document.querySelectorAll('#curve-options .setting-chip').forEach(btn => {
  btn.addEventListener('click', () => {
    settings.curveStyle = btn.dataset.curve;
    saveSettings();
    syncSettingsUI();
    drawCurvePreview();
    applySettings();
  });
});

/* Node fill buttons */
document.querySelectorAll('#fill-options .setting-chip').forEach(btn => {
  btn.addEventListener('click', () => {
    settings.nodeFill = btn.dataset.fill;
    saveSettings();
    syncSettingsUI();
    applySettings();
  });
});

/* Orientation buttons */
document.querySelectorAll('#orientation-options .setting-chip').forEach(btn => {
  btn.addEventListener('click', () => {
    settings.orientation = btn.dataset.orientation;
    saveSettings();
    syncSettingsUI();
    if (active !== null && sessions[active] && sessions[active].tree) {
      renderAST(sessions[active].tree);
    }
  });
});

/**
 * Build an SVG path string for two points using the active curve style.
 */
function curvedLink(sx, sy, tx, ty) {
  const mx = (sx + tx) / 2;
  const my = (sy + ty) / 2;
  switch (settings.curveStyle) {
    case 'straight':
      return `M ${sx} ${sy} L ${tx} ${ty}`;
    case 'step':
      return `M ${sx} ${sy} L ${mx} ${sy} L ${mx} ${ty} L ${tx} ${ty}`;
    case 'elbow':
      return `M ${sx} ${sy} L ${sx} ${my} L ${tx} ${my} L ${tx} ${ty}`;
    case 'arc': {
      const dx = tx - sx, dy = ty - sy;
      const r = Math.sqrt(dx * dx + dy * dy) * 0.7;
      return `M ${sx} ${sy} A ${r} ${r} 0 0 1 ${tx} ${ty}`;
    }
    case 'cubic':
    default:
      return `M ${sx} ${sy} C ${mx} ${sy}, ${mx} ${ty}, ${tx} ${ty}`;
  }
}

/**
 * Apply current settings to the live AST SVG without re-rendering.
 */
function applySettings() {
  const svg = d3.select('#ast-svg');

  // Links: curve style
  svg.selectAll('.ast-link')
    .transition().duration(300)
    .attr('d', function(d) {
      return curvedLink(d.source.x, d.source.y, d.target.x, d.target.y);
    });

  // Nodes: fill opacity
  svg.selectAll('.ast-node-rect')
    .transition().duration(300)
    .attr('fill-opacity', function() {
      if (settings.nodeFill === 'solid') return 0.85;
      if (settings.nodeFill === 'outline') return 0;
      return 0.2; // translucent
    });
}

/**
 * Draw the curve preview in the settings modal.
 */
function drawCurvePreview() {
  const svg = d3.select('#curve-preview');
  svg.selectAll('*').remove();
  const W = svg.node().clientWidth || 440;
  const H = 70;
  svg.attr('viewBox', `0 0 ${W} ${H}`);

  const points = [
    [30, 55], [W * 0.33, 15], [W * 0.66, 55], [W - 30, 15]
  ];

  // Draw links between consecutive points
  for (let i = 0; i < points.length - 1; i++) {
    const [sx, sy] = points[i];
    const [tx, ty] = points[i + 1];
    svg.append('path')
      .attr('class', 'preview-path')
      .attr('d', curvedLink(sx, sy, tx, ty));
  }

  // Draw dots
  for (const [x, y] of points) {
    svg.append('circle')
      .attr('class', 'preview-dot')
      .attr('cx', x).attr('cy', y).attr('r', 4);
  }
}

/* ─── Shared zoom state for AST ──────────────────────────────────── */
let currentZoom = null;
let currentSvg = null;

/* ═══════════════════════════════════════════════════════════════════
   DATA LOADING
   ═══════════════════════════════════════════════════════════════════ */

/* Pending data keyed by tok basename for pairing */
const tokData = {};   // tok basename → { input, tokens }
const astData = {};   // tok basename → { input, tree }

/** Extract basename from a path (e.g. "viz/tokenizations/tok_1.json" → "tok_1.json") */
function basename(path) {
  if (!path) return '';
  return path.split('/').pop();
}

function getDataRoot() {
  const params = new URLSearchParams(window.location.search);
  return params.get('data') || '.';
}

async function loadManifest() {
  const root = getDataRoot();
  // Load tokenization manifest
  try {
    const res = await fetch(`${root}/tokenizations/manifest.json`);
    if (res.ok) {
      const files = await res.json();
      for (const fname of files) {
        try {
          const r = await fetch(`${root}/tokenizations/${fname}`);
          const data = await r.json();
          tokData[basename(fname)] = {
            input: data.input,
            tokens: data.tokens,
            name: fname,
          };
        } catch (_) {}
      }
    }
  } catch (_) {}

  // Load AST manifest
  try {
    const res = await fetch(`${root}/ast/manifest.json`);
    if (res.ok) {
      const files = await res.json();
      for (const fname of files) {
        try {
          const r = await fetch(`${root}/ast/${fname}`);
          const data = await r.json();
          astData[basename(data.tok_file)] = {
            input: data.input,
            tree: data.tree,
            name: fname,
            tok_file: data.tok_file,
          };
        } catch (_) {}
      }
    }
  } catch (_) {}

  // Pair them into sessions
  buildSessions();
}

function buildSessions() {
  // First, create sessions from AST files (which reference tok files)
  const paired = new Set();
  for (const [tokKey, ast] of Object.entries(astData)) {
    const tok = tokData[tokKey];
    if (tok) {
      paired.add(tokKey);
      addSession({
        input: ast.input || tok.input,
        tokens: tok.tokens,
        tree: ast.tree,
        tokName: tok.name,
        astName: ast.name,
      });
    } else {
      // AST without matching tokenization
      addSession({
        input: ast.input,
        tokens: null,
        tree: ast.tree,
        tokName: null,
        astName: ast.name,
      });
    }
  }
  // Add unpaired tokenizations
  for (const [key, tok] of Object.entries(tokData)) {
    if (!paired.has(key)) {
      addSession({
        input: tok.input,
        tokens: tok.tokens,
        tree: null,
        tokName: tok.name,
        astName: null,
      });
    }
  }
}

/* ─── File picker (manual load) ─────────────────────────────────── */
filePicker.addEventListener('change', async () => {
  for (const file of filePicker.files) {
    const text = await file.text();
    try {
      const data = JSON.parse(text);
      if (data.tree) {
        addSession({
          input: data.input,
          tokens: null,
          tree: data.tree,
          tokName: null,
          astName: file.name,
        });
      } else if (data.tokens) {
        addSession({
          input: data.input,
          tokens: data.tokens,
          tree: null,
          tokName: file.name,
          astName: null,
        });
      }
    } catch (e) {
      alert(`Could not parse ${file.name}: ${e.message}`);
    }
  }
  filePicker.value = '';
});

/* ─── Session management ─────────────────────────────────────────── */
function addSession(s) {
  const key = s.input + (s.tokName || '') + (s.astName || '');
  if (sessions.find(x => (x.input + (x.tokName || '') + (x.astName || '')) === key)) return;
  sessions.push(s);
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
    el.title = s.input || '(no input)';

    const label = s.input ? s.input.slice(0, 28) + (s.input.length > 28 ? '...' : '') : '(no input)';
    let badges = '';
    if (s.tokens) badges += '<span class="badge badge-tok">TOK</span>';
    if (s.tree)   badges += '<span class="badge badge-ast">AST</span>';
    el.innerHTML = escHtml(label) + badges;

    el.addEventListener('click', () => showSession(i));
    sessionList.appendChild(el);
  });
}

/* ─── Main render ────────────────────────────────────────────────── */
function showSession(idx) {
  active = idx;
  selectedIdx = null;
  renderSidebar();

  const s = sessions[idx];
  welcome.style.display = 'none';
  vizContainer.style.display = 'flex';
  detailPanel.style.display = 'none';
  astDetail.style.display = 'none';

  inputDisplay.textContent = s.input || '';

  // Enable/disable tabs
  const tokTab = document.querySelector('.tab[data-tab="tokens"]');
  const astTab = document.querySelector('.tab[data-tab="ast"]');
  tokTab.classList.toggle('disabled', !s.tokens);
  astTab.classList.toggle('disabled', !s.tree);

  // Render both panes if data exists
  if (s.tokens) {
    renderTokenStrip(s.tokens);
    renderLegend(s.tokens);
    renderArc(s.tokens);
  } else {
    tokenStrip.innerHTML = '';
    legendEl.innerHTML = '';
    d3.select('#arc-diagram').selectAll('*').remove();
  }

  // Activate preferred tab, falling back if data is missing
  const pref = getPreferredTab();
  if (pref === 'ast' && s.tree) {
    activateTab('ast');
  } else if (pref === 'tokens' && s.tokens) {
    activateTab('tokens');
  } else if (s.tokens) {
    activateTab('tokens');
  } else {
    activateTab('ast');
  }

  if (s.tree) {
    renderAST(s.tree);
  } else {
    d3.select('#ast-svg').selectAll('*').remove();
  }
}

/* ═══════════════════════════════════════════════════════════════════
   TOKENIZATION VISUALIZATION (preserved from original)
   ═══════════════════════════════════════════════════════════════════ */

function renderTokenStrip(tokens) {
  tokenStrip.innerHTML = '';
  tokens.forEach((tok, i) => {
    const chip = document.createElement('div');
    chip.className = 'token-chip';
    chip.dataset.idx = i;
    const bg = tokenColor(tok.type);
    chip.style.background = hexOpacity(bg, 0.18);
    chip.style.color = bg;

    const val = document.createElement('span');
    val.className = 'chip-value';
    val.textContent = tok.value !== '' ? tok.value : '\u2205';

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
  showTokenDetail(tok);
  highlightArcs(idx);
}

function showTokenDetail(tok) {
  const color = tokenColor(tok.type);
  detailType.textContent = tok.type;
  detailType.style.color = color;
  detailTable.innerHTML = `
    <tr><td>value</td><td><code>${escHtml(tok.value)}</code></td></tr>
    <tr><td>io_number</td><td>${tok.io_number === -1 ? '<span style="color:var(--muted)">\u2014</span>' : tok.io_number}</td></tr>
    <tr><td>index</td><td>${selectedIdx}</td></tr>
  `;
  detailPanel.style.display = 'block';
}

function renderLegend(tokens) {
  const seen = [...new Set(tokens.map(t => t.type))];
  legendEl.innerHTML = '';
  seen.forEach(type => {
    const item = document.createElement('div');
    item.className = 'legend-item';
    item.innerHTML = `<div class="legend-swatch" style="background:${tokenColor(type)}"></div>${type.replace('TOK_', '')}`;
    item.addEventListener('click', () => highlightByType(type));
    legendEl.appendChild(item);
  });
}

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

  const arcs = [];
  for (let i = 0; i < n - 1; i++) {
    arcs.push({ from: i, to: i + 1 });
  }
  for (let i = 0; i < n; i++) {
    for (let j = i + 2; j < n; j++) {
      if (tokens[i].type === tokens[j].type && tokens[i].type !== 'TOK_EOF') {
        arcs.push({ from: i, to: j, sameType: true });
      }
    }
  }

  const g = svg.append('g');
  g.selectAll('.arc-path')
    .data(arcs)
    .enter()
    .append('path')
    .attr('class', d => `arc-path arc-from-${d.from} arc-to-${d.to}${d.sameType ? ' arc-same-type' : ''}`)
    .attr('d', d => {
      const x1 = xScale(d.from), x2 = xScale(d.to);
      const span = Math.abs(x2 - x1);
      const ry  = -(d.sameType ? span * 0.55 : span * 0.38);
      return `M ${x1} ${yBase} C ${x1} ${yBase - ry}, ${x2} ${yBase - ry}, ${x2} ${yBase}`;
    })
    .attr('stroke', d => d.sameType
      ? tokenColor(tokens[d.from].type)
      : '#30363d')
    .attr('stroke-dasharray', d => d.sameType ? '4 3' : null)
    .attr('opacity', d => d.sameType ? 0.3 : 0.6)
    .style('display', d => (d.sameType && !showRelations) ? 'none' : null);

  g.selectAll('.arc-node')
    .data(tokens)
    .enter()
    .append('circle')
    .attr('class', 'arc-node')
    .attr('cx', (_, i) => xScale(i))
    .attr('cy', yBase)
    .attr('r', 7)
    .attr('fill', d => tokenColor(d.type))
    .attr('opacity', 0.85)
    .on('click', (_, d) => {
      const idx = tokens.indexOf(d);
      selectToken(idx, d);
    })
    .append('title').text(d => `${d.type}\n"${d.value}"`);

  g.selectAll('.arc-label')
    .data(tokens)
    .enter()
    .append('text')
    .attr('class', 'arc-label')
    .attr('x', (_, i) => xScale(i))
    .attr('y', yBase - 15)
    .text(d => d.value.length > 6 ? d.value.slice(0, 5) + '\u2026' : d.value || '\u2205');
}

function highlightArcs(idx) {
  d3.selectAll('.arc-path').classed('hi', false);
  d3.selectAll(`.arc-from-${idx}, .arc-to-${idx}`).classed('hi', true);
}

function highlightByType(type) {
  document.querySelectorAll('.token-chip').forEach((chip, i) => {
    const tok = sessions[active].tokens[i];
    chip.style.opacity = tok.type === type ? '1' : '0.25';
  });
  setTimeout(() => {
    document.querySelectorAll('.token-chip').forEach(c => c.style.opacity = '');
  }, 1800);
}

/* ═══════════════════════════════════════════════════════════════════
   AST VISUALIZATION — D3 interactive tree
   ═══════════════════════════════════════════════════════════════════ */

/**
 * Convert the JSON AST (recursive) into a d3-hierarchy-friendly structure.
 * Each node gets: { name, type, details, children }
 */
function astToHierarchy(node) {
  if (!node) return null;
  const h = {
    name: NODE_LABELS[node.type] || node.type,
    type: node.type,
    _raw: node,
    children: [],
  };

  if (node.type === 'NODE_COMMAND') {
    h.sublabel = (node.argv || []).join(' ');
    if (h.sublabel.length > 30) h.sublabel = h.sublabel.slice(0, 28) + '...';
    // Add redirs as leaf children
    if (node.redirs && node.redirs.length > 0) {
      for (const r of node.redirs) {
        h.children.push({
          name: r.type + ' ' + (r.target || ''),
          type: '_REDIR',
          _raw: r,
          children: [],
          isRedir: true,
        });
      }
    }
  } else if (node.type === 'NODE_PIPE' || node.type === 'NODE_AND' ||
             node.type === 'NODE_OR' || node.type === 'NODE_SEQUENCE') {
    const left = astToHierarchy(node.left);
    const right = astToHierarchy(node.right);
    if (left) h.children.push(left);
    if (right) h.children.push(right);
  } else {
    // group types: subshell, block, background
    const child = astToHierarchy(node.child);
    if (child) h.children.push(child);
    if (node.redirs && node.redirs.length > 0) {
      for (const r of node.redirs) {
        h.children.push({
          name: r.type + ' ' + (r.target || ''),
          type: '_REDIR',
          _raw: r,
          children: [],
          isRedir: true,
        });
      }
    }
  }
  return h;
}

/* Collapse / expand helpers */
function toggleNode(d) {
  if (d.children) {
    d._children = d.children;
    d.children = null;
  } else if (d._children) {
    d.children = d._children;
    d._children = null;
  }
}

/* ─── Persistent tree update function ────────────────────────────── */
let updateTree = () => {};

function renderAST(tree) {
  const container = document.getElementById('ast-container');
  const svg = d3.select('#ast-svg');
  svg.selectAll('*').remove();

  if (!tree) return;

  const hierData = astToHierarchy(tree);
  if (!hierData) return;

  const root = d3.hierarchy(hierData, d => d.children);
  // Store for collapse/expand toolbar buttons
  if (active !== null) sessions[active]._d3root = root.data;

  const W = container.clientWidth || 800;
  const H = container.clientHeight || 500;

  // Node dimensions
  const nodeW = 110;
  const nodeH = 50;

  const g = svg.append('g');

  // Zoom + pan
  const zoom = d3.zoom()
    .scaleExtent([0.2, 4])
    .on('zoom', (e) => g.attr('transform', e.transform));
  svg.call(zoom);
  currentZoom = zoom;
  currentSvg = svg;

  const treeFn = d3.tree();

  function calcLayout(src) {
    // Recompute hierarchy from data (to handle collapse)
    const newRoot = d3.hierarchy(sessions[active]._d3root, d => d.children);

    // Count leaves for sizing
    const leaves = newRoot.leaves().length;
    const depth = newRoot.height;

    if ((settings.orientation === 'horizontal')) {
      const treeH = Math.max(leaves * (nodeH + 20), H - 40);
      const treeW = Math.max((depth + 1) * (nodeW + 80), W - 40);
      treeFn.size([treeH, treeW]);
    } else {
      const treeW = Math.max(leaves * (nodeW + 30), W - 40);
      const treeH = Math.max((depth + 1) * (nodeH + 60), H - 40);
      treeFn.size([treeW, treeH]);
    }

    treeFn(newRoot);
    return newRoot;
  }

  updateTree = function(srcData) {
    const newRoot = calcLayout(srcData);
    const nodes = newRoot.descendants();
    const links = newRoot.links();

    // Swap x/y for horizontal layout
    if ((settings.orientation === 'horizontal')) {
      nodes.forEach(d => { const tmp = d.x; d.x = d.y + 60; d.y = tmp + 20; });
    } else {
      nodes.forEach(d => { d.x += 20; d.y += 40; });
    }

    /* ── Links ── */
    const link = g.selectAll('.ast-link').data(links, d => d.target.data.name + d.target.depth);

    link.exit().remove();

    const linkEnter = link.enter()
      .append('path')
      .attr('class', 'ast-link');

    link.merge(linkEnter)
      .transition().duration(400)
      .attr('d', d => curvedLink(d.source.x, d.source.y, d.target.x, d.target.y));

    /* ── Nodes ── */
    const node = g.selectAll('.ast-node').data(nodes, d => d.data.name + d.depth + (d.parent ? d.parent.data.name : ''));

    node.exit().remove();

    const nodeEnter = node.enter()
      .append('g')
      .attr('class', 'ast-node')
      .attr('transform', d => `translate(${d.x},${d.y})`)
      .on('click', (event, d) => {
        event.stopPropagation();
        if (event.shiftKey) {
          // Shift+click to collapse/expand
          toggleNode(d.data);
          updateTree(d.data);
        } else {
          showASTDetail(d.data);
          // Highlight selected
          g.selectAll('.ast-node').classed('selected', false);
          d3.select(event.currentTarget).classed('selected', true);
          // Highlight path to root
          highlightPath(d);
        }
      });

    // Background rect
    nodeEnter.append('rect')
      .attr('class', 'ast-node-rect')
      .attr('width', d => d.data.isRedir ? 90 : nodeW)
      .attr('height', d => {
        if (d.data.isRedir) return 32;
        return d.data.sublabel ? nodeH + 14 : nodeH;
      })
      .attr('x', d => -(d.data.isRedir ? 90 : nodeW) / 2)
      .attr('y', d => {
        const h = d.data.isRedir ? 32 : (d.data.sublabel ? nodeH + 14 : nodeH);
        return -h / 2;
      })
      .attr('fill', d => d.data.isRedir ? '#30363d' : nodeColor(d.data.type))
      .attr('fill-opacity', () => {
        if (settings.nodeFill === 'solid') return 0.85;
        if (settings.nodeFill === 'outline') return 0;
        return 0.2;
      })
      .attr('stroke', d => d.data.isRedir ? '#56d364' : nodeColor(d.data.type));

    // Main label
    nodeEnter.append('text')
      .attr('class', 'ast-node-label')
      .attr('dy', d => d.data.sublabel ? '-0.4em' : '0')
      .text(d => d.data.name);

    // Sublabel (argv for commands)
    nodeEnter.filter(d => d.data.sublabel)
      .append('text')
      .attr('class', 'ast-node-sublabel')
      .attr('dy', '1.1em')
      .text(d => d.data.sublabel);

    // Collapse indicator
    nodeEnter.filter(d => d.data.children && d.data.children.length > 0 || d.data._children)
      .append('text')
      .attr('class', 'ast-collapse-icon')
      .attr('dy', d => {
        const h = d.data.sublabel ? nodeH + 14 : nodeH;
        return h / 2 + 12;
      })
      .text(d => d.data._children ? '+' : '');

    // Merge + transition
    const nodeAll = node.merge(nodeEnter);

    nodeAll.transition().duration(400)
      .attr('transform', d => `translate(${d.x},${d.y})`);

    // Update collapse icons
    nodeAll.select('.ast-collapse-icon')
      .text(d => d.data._children ? `+${d.data._children.length}` : '');
  };

  updateTree(root.data);

  // Apply link color + gradient settings after first render
  setTimeout(() => applySettings(), 420);

  // Auto-fit: compute bounding box of all nodes and center within viewport
  requestAnimationFrame(() => {
    const bbox = g.node().getBBox();
    if (!bbox.width || !bbox.height) return;
    const pad = 40;
    const scaleX = (W - pad * 2) / bbox.width;
    const scaleY = (H - pad * 2) / bbox.height;
    const s = Math.min(scaleX, scaleY, 1.2);
    const tx = (W - bbox.width * s) / 2 - bbox.x * s;
    const ty = (H - bbox.height * s) / 2 - bbox.y * s;
    svg.call(zoom.transform, d3.zoomIdentity.translate(tx, ty).scale(s));
  });
}

function highlightPath(d) {
  d3.selectAll('.ast-link').classed('highlighted', false);
  // Walk up to root
  let current = d;
  const pathNodes = new Set();
  while (current) {
    pathNodes.add(current);
    current = current.parent;
  }
  d3.selectAll('.ast-link').classed('highlighted', link =>
    pathNodes.has(link.source) && pathNodes.has(link.target)
  );
}

function showASTDetail(data) {
  const raw = data._raw;
  const color = data.isRedir ? '#56d364' : nodeColor(data.type);

  astDetailType.textContent = data.isRedir ? 'Redirection' : data.type;
  astDetailType.style.color = color;

  let html = '<table>';

  if (data.isRedir) {
    html += `<tr><td>operator</td><td><code class="redir-op">${escHtml(raw.type)}</code></td></tr>`;
    html += `<tr><td>fd</td><td>${raw.fd === -1 ? '<span style="color:var(--muted)">default</span>' : raw.fd}</td></tr>`;
    html += `<tr><td>target</td><td><code class="redir-target">${escHtml(raw.target)}</code></td></tr>`;
    if (raw.heredoc_delim) {
      html += `<tr><td>heredoc delim</td><td><code>${escHtml(raw.heredoc_delim)}</code></td></tr>`;
      html += `<tr><td>heredoc quoted</td><td>${raw.heredoc_quoted ? 'yes' : 'no'}</td></tr>`;
    }
  } else if (data.type === 'NODE_COMMAND') {
    const argv = raw.argv || [];
    html += `<tr><td>command</td><td><code>${escHtml(argv[0] || '(empty)')}</code></td></tr>`;
    if (argv.length > 1) {
      html += `<tr><td>arguments</td><td>${argv.slice(1).map(a => `<code>${escHtml(a)}</code>`).join(' ')}</td></tr>`;
    }
    html += `<tr><td>argc</td><td>${raw.argc}</td></tr>`;

    const assigns = raw.assignments || [];
    if (assigns.length > 0) {
      html += `<tr><td>assignments</td><td>${assigns.map(a => `<code>${escHtml(a)}</code>`).join('<br/>')}</td></tr>`;
    }

    const redirs = raw.redirs || [];
    if (redirs.length > 0) {
      html += '<tr><td>redirections</td><td><ul class="redir-list">';
      for (const r of redirs) {
        const fd = r.fd === -1 ? '' : `<span class="redir-fd">${r.fd}</span>`;
        html += `<li>${fd}<span class="redir-op">${escHtml(r.type)}</span> <span class="redir-target">${escHtml(r.target)}</span></li>`;
      }
      html += '</ul></td></tr>';
    }
  } else if (data.type === 'NODE_PIPE' || data.type === 'NODE_AND' ||
             data.type === 'NODE_OR' || data.type === 'NODE_SEQUENCE') {
    const opSymbols = {
      NODE_PIPE: '|', NODE_AND: '&&', NODE_OR: '||', NODE_SEQUENCE: ';',
    };
    html += `<tr><td>operator</td><td><code>${opSymbols[data.type] || data.type}</code></td></tr>`;
    html += `<tr><td>structure</td><td>binary (left, right)</td></tr>`;

    // Show children summary
    const leftType = raw.left ? raw.left.type : 'null';
    const rightType = raw.right ? raw.right.type : 'null';
    html += `<tr><td>left child</td><td><span style="color:${nodeColor(leftType)}">${leftType}</span></td></tr>`;
    html += `<tr><td>right child</td><td><span style="color:${nodeColor(rightType)}">${rightType}</span></td></tr>`;
  } else {
    // Group types
    const groupNames = {
      NODE_SUBSHELL: '( ... )', NODE_BLOCK: '{ ... ; }', NODE_BACKGROUND: '... &',
    };
    html += `<tr><td>syntax</td><td><code>${groupNames[data.type] || data.type}</code></td></tr>`;
    html += `<tr><td>structure</td><td>group (child + redirections)</td></tr>`;

    const childType = raw.child ? raw.child.type : 'null';
    html += `<tr><td>child</td><td><span style="color:${nodeColor(childType)}">${childType}</span></td></tr>`;

    const redirs = raw.redirs || [];
    if (redirs.length > 0) {
      html += '<tr><td>redirections</td><td><ul class="redir-list">';
      for (const r of redirs) {
        const fd = r.fd === -1 ? '' : `<span class="redir-fd">${r.fd}</span>`;
        html += `<li>${fd}<span class="redir-op">${escHtml(r.type)}</span> <span class="redir-target">${escHtml(r.target)}</span></li>`;
      }
      html += '</ul></td></tr>';
    }
  }

  html += '</table>';

  // Tip
  html += '<p style="color:var(--muted);font-size:.72rem;margin-top:.8rem">Shift+click a node to collapse/expand its subtree</p>';

  astDetailBody.innerHTML = html;
  astDetail.style.display = 'block';
}

/* ═══════════════════════════════════════════════════════════════════
   UTILITIES
   ═══════════════════════════════════════════════════════════════════ */

function escHtml(s) {
  return (s || '').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

function hexOpacity(cssColor, alpha) {
  return `color-mix(in srgb, ${cssColor} ${Math.round(alpha * 100)}%, transparent)`;
}

/* ─── Boot ───────────────────────────────────────────────────────── */
loadManifest();
