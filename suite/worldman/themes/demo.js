import { DEMO } from './demo-data.js';

const select = document.getElementById('theme-select');
const groupFilter = document.getElementById('group-filter');
const entireWindow = document.getElementById('entire-window');
const modeToggle = document.getElementById('mode-toggle');
const meta = document.getElementById('meta');
const tokenSample = document.getElementById('token-sample');

const GROUP_ORDER = { minor: 0, vibe: 1, country: 2 };
const TYPE_ORDER = { light: 0, dark: 1 };
const ENTIRE_KEY = 'worldman-themes-demo-entire';
const MODE_KEY = 'worldman-themes-demo-mode';
const MODE_LABEL = { light: 'Light', dark: 'Dark' };

/** When true, Light/Dark paints index chrome with the theme. */
let entire = true;
/** light | dark — chrome when Entire on; theme-type target when Entire off. */
let mode = 'light';

tokenSample.innerHTML = [
  `<span class="tok-comment">// theme token sample</span>`,
  `<span class="tok-keyword">export function</span> <span class="tok-fn">greet</span><span class="tok-punct">(</span>name<span class="tok-punct">:</span> <span class="tok-type">string</span><span class="tok-punct">)</span> <span class="tok-punct">{</span>`,
  `  <span class="tok-keyword">const</span> n <span class="tok-punct">=</span> <span class="tok-number">42</span><span class="tok-punct">;</span>`,
  `  <span class="tok-keyword">return</span> <span class="tok-string">\`hello \${name} / \${n}\`</span><span class="tok-punct">;</span>`,
  `<span class="tok-punct">}</span>`,
].join('\n');

function compareThemes(a, b) {
  const td = (TYPE_ORDER[a.type] ?? 9) - (TYPE_ORDER[b.type] ?? 9);
  if (td) return td;
  const gd = (GROUP_ORDER[a.group] ?? 9) - (GROUP_ORDER[b.group] ?? 9);
  if (gd) return gd;
  return a.label.localeCompare(b.label, 'en');
}

function catalogEntry(value = select.value) {
  if (!value) return null;
  const [group, paletteKey] = value.split(':');
  return DEMO.catalog.find((c) => c.group === group && c.paletteKey === paletteKey) || null;
}

function optionValue(entry) {
  return `${entry.group}:${entry.paletteKey}`;
}

/** Same family (+ prefer same group) of the opposite / requested type. */
function findSibling(entry, type) {
  if (!entry) return null;
  return (
    DEMO.catalog.find(
      (c) => c.family === entry.family && c.group === entry.group && c.type === type,
    ) ||
    DEMO.catalog.find((c) => c.family === entry.family && c.type === type) ||
    null
  );
}

function persist() {
  try {
    localStorage.setItem(ENTIRE_KEY, entire ? '1' : '0');
    localStorage.setItem(MODE_KEY, mode);
  } catch {
    /* ignore */
  }
}

function applyChrome() {
  if (entire) {
    document.documentElement.dataset.mode = mode;
  }
  /* Entire off: leave data-mode frozen (do not touch framework). */
  modeToggle.textContent = MODE_LABEL[mode];
  modeToggle.setAttribute('aria-pressed', mode === 'dark' ? 'true' : 'false');
  modeToggle.title = entire
    ? `Entire window: ${MODE_LABEL[mode]} chrome + theme`
    : `Theme only: switch to ${MODE_LABEL[mode === 'light' ? 'dark' : 'light']} (chrome frozen)`;
  entireWindow.checked = entire;
}

function selectThemeEntry(entry) {
  if (!entry) return;
  const value = optionValue(entry);
  if (![...select.options].some((o) => o.value === value)) {
    const opt = document.createElement('option');
    opt.value = value;
    opt.textContent = `${entry.label} · ${entry.group}`;
    select.appendChild(opt);
  }
  select.value = value;
  applyTheme();
}

function setVars(prefix, obj, root = document.documentElement) {
  if (!obj) return;
  for (const [k, v] of Object.entries(obj)) {
    root.style.setProperty(`--${prefix}-${k}`, v);
  }
}

function applyTheme() {
  const entry = catalogEntry();
  if (!entry) return;
  const pack = DEMO.groups[entry.group];
  if (!pack) return;
  const { paletteKey } = entry;

  setVars('ui', pack.ui[paletteKey]);
  setVars('web', pack.web[paletteKey]);
  setVars('erp', pack.erp[paletteKey]);

  const tok = pack.token[paletteKey] || {};
  setVars('tok', tok);
  const ui = pack.ui[paletteKey] || {};
  document.documentElement.style.setProperty(
    '--tok-bg',
    ui.surfaceBg || ui.windowBg || '#1e1e1e',
  );

  const locs = entry.locales ? ` · locales ${entry.locales.replace(/\|/g, ', ')}` : '';
  meta.textContent = `${entry.id} · ${entry.type} · ${entry.family}${locs} · ${DEMO.catalog.length} themes`;

  mode = entry.type === 'dark' ? 'dark' : 'light';
  applyChrome();
  persist();
}

function fillSelect() {
  const g = groupFilter.value;
  const items = DEMO.catalog
    .filter((c) => g === 'all' || c.group === g)
    .slice()
    .sort(compareThemes);

  const prev = select.value;
  select.innerHTML = '';

  let lightGroup = null;
  let darkGroup = null;
  for (const c of items) {
    let og = c.type === 'dark' ? darkGroup : lightGroup;
    if (!og) {
      og = document.createElement('optgroup');
      og.label = c.type === 'dark' ? 'Dark' : 'Light';
      select.appendChild(og);
      if (c.type === 'dark') darkGroup = og;
      else lightGroup = og;
    }
    const opt = document.createElement('option');
    opt.value = optionValue(c);
    opt.textContent = `${c.label} · ${c.group}`;
    opt.dataset.type = c.type;
    opt.dataset.family = c.family;
    og.appendChild(opt);
  }

  if (prev && [...select.options].some((o) => o.value === prev)) {
    select.value = prev;
  } else if (select.options[0]) {
    select.value = select.options[0].value;
  }
  applyTheme();
}

function toggleMode() {
  const next = mode === 'light' ? 'dark' : 'light';
  const sibling = findSibling(catalogEntry(), next);
  if (sibling) {
    selectThemeEntry(sibling);
    return;
  }
  mode = next;
  applyChrome();
  persist();
}

function setEntire(on) {
  entire = !!on;
  if (entire) {
    const cur = catalogEntry();
    mode = cur?.type === 'dark' ? 'dark' : mode;
    applyChrome();
  } else {
    /* Freeze framework at current chrome; Light/Dark will only move themes. */
    const cur = catalogEntry();
    if (cur) mode = cur.type === 'dark' ? 'dark' : 'light';
    applyChrome();
  }
  persist();
}

groupFilter.addEventListener('change', fillSelect);
select.addEventListener('change', applyTheme);
entireWindow.addEventListener('change', () => setEntire(entireWindow.checked));
modeToggle.addEventListener('click', toggleMode);

try {
  const savedEntire = localStorage.getItem(ENTIRE_KEY);
  if (savedEntire === '0' || savedEntire === '1') entire = savedEntire === '1';
  const savedMode = localStorage.getItem(MODE_KEY);
  if (savedMode === 'light' || savedMode === 'dark') mode = savedMode;
  /* Migrate old Auto chrome pref. */
  if (savedMode === 'auto') mode = 'light';
} catch {
  /* ignore */
}

if (entire) {
  document.documentElement.dataset.mode = mode;
} else {
  document.documentElement.dataset.mode = 'light';
}
entireWindow.checked = entire;
applyChrome();
fillSelect();
