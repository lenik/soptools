import { DEMO } from './demo-data.js';

const select = document.getElementById('theme-select');
const groupFilter = document.getElementById('group-filter');
const modeToggle = document.getElementById('mode-toggle');
const meta = document.getElementById('meta');
const tokenSample = document.getElementById('token-sample');

const GROUP_ORDER = { minor: 0, vibe: 1, country: 2 };
const TYPE_ORDER = { light: 0, dark: 1 };
const MODE_KEY = 'worldman-themes-demo-chrome';
const MODE_CYCLE = ['light', 'dark', 'auto'];
const MODE_LABEL = { light: 'Light', dark: 'Dark', auto: 'Auto' };

/** User preference: light | dark | auto */
let chromePref = 'auto';

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

function selectedThemeType() {
  if (!select.value) return 'light';
  const [group, paletteKey] = select.value.split(':');
  const entry = DEMO.catalog.find((c) => c.group === group && c.paletteKey === paletteKey);
  return entry?.type === 'dark' ? 'dark' : 'light';
}

/** Resolve effective chrome appearance from preference + theme. */
function resolvedChromeMode() {
  return chromePref === 'auto' ? selectedThemeType() : chromePref;
}

function applyChrome() {
  const mode = resolvedChromeMode();
  document.documentElement.dataset.mode = mode;
  modeToggle.textContent = MODE_LABEL[chromePref];
  modeToggle.setAttribute('aria-pressed', chromePref === 'dark' ? 'true' : 'false');
  modeToggle.title =
    chromePref === 'auto'
      ? 'Index chrome follows theme (Auto)'
      : `Index chrome locked to ${MODE_LABEL[chromePref]}`;
}

function setChromePref(pref) {
  chromePref = MODE_CYCLE.includes(pref) ? pref : 'auto';
  try {
    localStorage.setItem(MODE_KEY, chromePref);
  } catch {
    /* ignore */
  }
  applyChrome();
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
    opt.value = `${c.group}:${c.paletteKey}`;
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

function setVars(prefix, obj, root = document.documentElement) {
  if (!obj) return;
  for (const [k, v] of Object.entries(obj)) {
    root.style.setProperty(`--${prefix}-${k}`, v);
  }
}

function applyTheme() {
  if (!select.value) return;
  const [group, paletteKey] = select.value.split(':');
  const entry = DEMO.catalog.find((c) => c.group === group && c.paletteKey === paletteKey);
  const pack = DEMO.groups[group];
  if (!pack || !entry) return;

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

  if (chromePref === 'auto') applyChrome();
}

groupFilter.addEventListener('change', fillSelect);
select.addEventListener('change', applyTheme);
modeToggle.addEventListener('click', () => {
  const i = MODE_CYCLE.indexOf(chromePref);
  setChromePref(MODE_CYCLE[(i + 1) % MODE_CYCLE.length]);
});

let initial = 'auto';
try {
  const saved = localStorage.getItem(MODE_KEY);
  if (MODE_CYCLE.includes(saved)) initial = saved;
} catch {
  /* ignore */
}
setChromePref(initial);
fillSelect();
