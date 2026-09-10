#!/usr/bin/env node
/**
 * Build suite/worldman/themes/ from a minor-themes checkout:
 *   catalog/*.md + {minor,vibe,country}/{ui,token,web,erp}-palettes.mjs
 *
 * Usage:
 *   node tools/extract-worldman-themes.mjs [/path/to/minor-themes]
 */
import {
    mkdirSync,
    writeFileSync,
    readFileSync,
    rmSync,
    readdirSync,
    existsSync,
} from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const root = join(__dirname, '..');
const MT = process.argv[2] || '/home/cursor/archive/minor-themes';
const outRoot = join(root, 'suite/worldman/themes');

function extractConstObject(src, name) {
    const startRe = new RegExp(`(?:export\\s+)?const\\s+${name}\\s*=\\s*\\{`);
    const m = startRe.exec(src);
    if (!m) {
        throw new Error(`const ${name} not found`);
    }
    let i = m.index + m[0].length - 1;
    let depth = 0;
    const start = i;
    for (; i < src.length; i++) {
        const ch = src[i];
        if (ch === '{') {
            depth++;
        } else if (ch === '}') {
            depth--;
            if (depth === 0) {
                let lit = src.slice(start, i + 1);
                lit = lit.replace(/,\s*\.\.\.[A-Za-z_][A-Za-z0-9_]*/g, '');
                return Function(`"use strict"; return (${lit});`)();
            }
        }
    }
    throw new Error(`unclosed ${name}`);
}

function extractBlock(text, a, b) {
    const i = text.indexOf(a);
    if (i < 0) {
        return '';
    }
    const j = text.indexOf(b, i + a.length);
    return (j < 0 ? text.slice(i) : text.slice(i, j)).replace(a, '').trim();
}

function keyLit(k) {
    return /^[A-Za-z_][A-Za-z0-9_]*$/.test(k) ? k : JSON.stringify(k);
}

function serializePaletteObject(title, obj, exportName) {
    const body = Object.keys(obj)
        .map((k) => {
            const entry = obj[k];
            const fields = Object.entries(entry)
                .map(([fk, fv]) => `    ${keyLit(fk)}: ${JSON.stringify(fv)},`)
                .join('\n');
            return `  ${keyLit(k)}: {\n${fields}\n  },`;
        })
        .join('\n');
    return `/**
 * ${title} — grouped color definitions (HSL strings).
 * Hand-adapted from minor-themes; no VS Code generator output.
 */
export const ${exportName} = {\n${body}\n};\n`;
}

function writePalettes(dir, packs, label) {
    mkdirSync(dir, { recursive: true });
    writeFileSync(
        join(dir, 'ui-palettes.mjs'),
        serializePaletteObject(`${label} UI (UiTheme roles)`, packs.ui, 'uiPalettes'),
    );
    writeFileSync(
        join(dir, 'token-palettes.mjs'),
        serializePaletteObject(`${label} syntax tokens`, packs.tokens, 'tokenPalettes'),
    );
    writeFileSync(
        join(dir, 'web-palettes.mjs'),
        serializePaletteObject(`${label} WorldMan web styleclasses`, packs.web, 'webPalettes'),
    );
    writeFileSync(
        join(dir, 'erp-palettes.mjs'),
        serializePaletteObject(`${label} ERP semantic colors`, packs.erp, 'erpPalettes'),
    );
}

function mapWeb(ui) {
    const fg = ui.windowFg;
    const bg = ui.windowBg;
    const surface = ui.surfaceBg || bg;
    const panel = ui.panelBg || bg;
    const primary = ui.actionFg;
    return {
        background: bg,
        foreground: fg,
        primary,
        'primary-foreground': 'hsl(0, 0%, 100%)',
        card: ui.cardBg || surface,
        'card-foreground': fg,
        accent: ui.selectedBg || ui.intervalBg || panel,
        'accent-foreground': ui.intervalFg || fg,
        'muted-foreground': ui.mutedFg,
        border: ui.border,
        ring: primary,
        destructive: ui.gitDeleted,
        success: ui.gitUntracked,
        warning: ui.gitModified,
    };
}

function mapErp(ui) {
    const fg = ui.windowFg;
    const bg = ui.windowBg;
    const panel = ui.panelBg || bg;
    const surface = ui.surfaceBg || bg;
    return {
        page: bg,
        'page-foreground': fg,
        panel,
        'panel-foreground': fg,
        header: panel,
        toolbar: ui.promptBg || surface,
        link: ui.actionFg,
        button: ui.actionFg,
        'button-foreground': 'hsl(0, 0%, 100%)',
        danger: ui.gitDeleted,
        'danger-foreground': 'hsl(0, 0%, 100%)',
        success: ui.gitUntracked,
        'success-foreground': 'hsl(0, 0%, 100%)',
        warning: ui.gitModified,
        'warning-foreground': fg,
        'table-header': panel,
        'table-border': ui.border || ui.grid,
        'row-alt': ui.intervalBg || ui.selectedBg || panel,
        'form-border': ui.border,
        'form-focus': ui.actionFg,
        'status-neutral': ui.mutedFg,
        'status-active': ui.actionFg,
        'status-done': ui.gitUntracked,
        'status-error': ui.gitDeleted,
        'amount-in': ui.gitUntracked,
        'amount-out': ui.gitDeleted,
    };
}

function uiToWebErp(uiMap) {
    const web = {};
    const erp = {};
    for (const [k, ui] of Object.entries(uiMap)) {
        web[k] = mapWeb(ui);
        erp[k] = mapErp(ui);
    }
    return { web, erp };
}

function tokensByPaletteKey(fileList, keyMap, tokenPalettes) {
    const out = {};
    for (const file of fileList) {
        const id = file.replace(/-color-theme\.json$/, '');
        const pk = keyMap[id];
        const tok = tokenPalettes[file];
        if (pk && tok) {
            out[pk] = tok;
        }
    }
    return out;
}

const genSrc = readFileSync(join(MT, 'scripts/generate-themes.mjs'), 'utf8');
const countrySrc = readFileSync(join(MT, 'scripts/country-palettes.mjs'), 'utf8');
const tokenSrc = readFileSync(join(MT, 'scripts/token-palettes.mjs'), 'utf8');
const ctokSrc = readFileSync(join(MT, 'scripts/country-token-palettes.mjs'), 'utf8');
const readme = readFileSync(join(MT, 'README.md'), 'utf8');
const pkg = JSON.parse(readFileSync(join(MT, 'package.json'), 'utf8'));

const aestheticUi = extractConstObject(genSrc, 'palettes');
const countryUi = extractConstObject(countrySrc, 'countryPalettes');
const tokenPalettes = {
    ...extractConstObject(tokenSrc, 'tokenPalettes'),
    ...extractConstObject(ctokSrc, 'countryDarkTokenPalettes'),
};

const MINOR_KEYS = new Set([
    'innocent',
    'darkInnocent',
    'maiden',
    'darkMaiden',
    'girl',
    'darkGirl',
    'morandi',
    'darkMorandi',
    'lgbtq',
    'darkLgbtq',
    'lesbian',
    'darkLesbian',
]);
const VIBE_KEYS = new Set([
    'msdos',
    'matrix2',
    'xfiles',
    'darkXfiles',
    'onlyYesterday',
    'darkOnlyYesterday',
]);

const minorUi = {};
const vibeUi = {};
for (const [k, v] of Object.entries(aestheticUi)) {
    if (MINOR_KEYS.has(k)) {
        minorUi[k] = v;
    } else if (VIBE_KEYS.has(k)) {
        vibeUi[k] = v;
    }
}

const minorFiles = [
    'innocent-color-theme.json',
    'dark-innocent-color-theme.json',
    'light-maiden-color-theme.json',
    'dark-maiden-color-theme.json',
    'light-girl-color-theme.json',
    'dark-girl-color-theme.json',
    'light-morandi-color-theme.json',
    'dark-morandi-color-theme.json',
    'light-lgbtq-color-theme.json',
    'dark-lgbtq-color-theme.json',
    'light-lesbian-color-theme.json',
    'dark-lesbian-color-theme.json',
];
const vibeFiles = [
    'ms-dos-color-theme.json',
    'matrix-ii-color-theme.json',
    'light-x-files-color-theme.json',
    'dark-x-files-color-theme.json',
    'light-only-yesterday-color-theme.json',
    'dark-only-yesterday-color-theme.json',
];

const minorKeyMap = {
    innocent: 'innocent',
    'dark-innocent': 'darkInnocent',
    'light-maiden': 'maiden',
    'dark-maiden': 'darkMaiden',
    'light-girl': 'girl',
    'dark-girl': 'darkGirl',
    'light-morandi': 'morandi',
    'dark-morandi': 'darkMorandi',
    'light-lgbtq': 'lgbtq',
    'dark-lgbtq': 'darkLgbtq',
    'light-lesbian': 'lesbian',
    'dark-lesbian': 'darkLesbian',
};
const vibeKeyMap = {
    'ms-dos': 'msdos',
    'matrix-ii': 'matrix2',
    'light-x-files': 'xfiles',
    'dark-x-files': 'darkXfiles',
    'light-only-yesterday': 'onlyYesterday',
    'dark-only-yesterday': 'darkOnlyYesterday',
};
const countryKeyMap = {};
{
    const re = /\["([^"]+)",\s*"(\w+)",\s*(true|false),\s*"([^"]+)"\]/g;
    let m;
    while ((m = re.exec(countrySrc))) {
        countryKeyMap[m[4].replace(/-color-theme\.json$/, '')] = m[2];
    }
}

const minorWE = uiToWebErp(minorUi);
const vibeWE = uiToWebErp(vibeUi);
const countryWE = uiToWebErp(countryUi);

if (existsSync(outRoot)) {
    for (const f of readdirSync(outRoot)) {
        if (f.endsWith('.theme') || f === 'palettes' || f.endsWith('.json')) {
            rmSync(join(outRoot, f), { recursive: true, force: true });
        }
    }
}
for (const sub of ['catalog', 'minor', 'country', 'vibe']) {
    mkdirSync(join(outRoot, sub), { recursive: true });
}

writePalettes(
    join(outRoot, 'minor'),
    {
        ui: minorUi,
        tokens: tokensByPaletteKey(minorFiles, minorKeyMap, tokenPalettes),
        web: minorWE.web,
        erp: minorWE.erp,
    },
    'minor',
);
writePalettes(
    join(outRoot, 'vibe'),
    {
        ui: vibeUi,
        tokens: tokensByPaletteKey(vibeFiles, vibeKeyMap, tokenPalettes),
        web: vibeWE.web,
        erp: vibeWE.erp,
    },
    'vibe',
);
writePalettes(
    join(outRoot, 'country'),
    {
        ui: countryUi,
        tokens: tokensByPaletteKey(
            Object.keys(countryKeyMap).map((id) => `${id}-color-theme.json`),
            countryKeyMap,
            tokenPalettes,
        ),
        web: countryWE.web,
        erp: countryWE.erp,
    },
    'country',
);

const catalogs = [
    {
        file: 'Innocent.md',
        group: 'minor',
        meaning: 'Innocent — pure, gentle, unassuming.',
        body: [
            ['Innocent (light)', extractBlock(readme, '**Innocent (light)**', '**Dark Innocent**')],
            ['Dark Innocent', extractBlock(readme, '**Dark Innocent**', '### Maiden')],
        ],
        variants: [
            { id: 'innocent', label: 'Innocent', type: 'light', paletteKey: 'innocent' },
            {
                id: 'dark-innocent',
                label: 'Dark Innocent',
                type: 'dark',
                paletteKey: 'darkInnocent',
            },
        ],
    },
    {
        file: 'Maiden.md',
        group: 'minor',
        meaning:
            'Maiden — young woman; also the romantic, delicate sense of “maidenly.”',
        body: [
            ['Light Maiden', extractBlock(readme, '**Light Maiden**', '**Dark Maiden**')],
            ['Dark Maiden', extractBlock(readme, '**Dark Maiden**', '### Girl')],
        ],
        variants: [
            { id: 'light-maiden', label: 'Light Maiden', type: 'light', paletteKey: 'maiden' },
            { id: 'dark-maiden', label: 'Dark Maiden', type: 'dark', paletteKey: 'darkMaiden' },
        ],
    },
    {
        file: 'Girl.md',
        group: 'minor',
        meaning:
            'Girl — youthful, bold, playful (not “girly” as in childish — more vivid and confident).',
        body: [
            ['Light Girl', extractBlock(readme, '**Light Girl**', '**Dark Girl**')],
            ['Dark Girl', extractBlock(readme, '**Dark Girl**', '### Morandi')],
        ],
        variants: [
            { id: 'light-girl', label: 'Light Girl', type: 'light', paletteKey: 'girl' },
            { id: 'dark-girl', label: 'Dark Girl', type: 'dark', paletteKey: 'darkGirl' },
        ],
    },
    {
        file: 'Morandi.md',
        group: 'minor',
        meaning:
            'Named after Italian painter Giorgio Morandi — dusty, muted, harmonious grays.',
        body: [
            ['Light Morandi', extractBlock(readme, '**Light Morandi**', '**Dark Morandi**')],
            ['Dark Morandi', extractBlock(readme, '**Dark Morandi**', '### LGBTQ')],
        ],
        variants: [
            { id: 'light-morandi', label: 'Light Morandi', type: 'light', paletteKey: 'morandi' },
            { id: 'dark-morandi', label: 'Dark Morandi', type: 'dark', paletteKey: 'darkMorandi' },
        ],
    },
    {
        file: 'LGBTQ.md',
        group: 'minor',
        meaning:
            'Colors drawn from the rainbow pride flag (Gilbert Baker design). Stripe hues as accents, not full-background stripes.',
        body: [
            ['Light LGBTQ', extractBlock(readme, '**Light LGBTQ**', '**Dark LGBTQ**')],
            ['Dark LGBTQ', extractBlock(readme, '**Dark LGBTQ**', '### Lesbian')],
        ],
        variants: [
            { id: 'light-lgbtq', label: 'Light LGBTQ', type: 'light', paletteKey: 'lgbtq' },
            { id: 'dark-lgbtq', label: 'Dark LGBTQ', type: 'dark', paletteKey: 'darkLgbtq' },
        ],
    },
    {
        file: 'Lesbian.md',
        group: 'minor',
        meaning:
            'An inward color language — the warmth and closeness of love between women.',
        body: [
            ['Light Lesbian', extractBlock(readme, '**Light Lesbian**', '**Dark Lesbian**')],
            ['Dark Lesbian', extractBlock(readme, '**Dark Lesbian**', '### MS-DOS')],
        ],
        variants: [
            { id: 'light-lesbian', label: 'Light Lesbian', type: 'light', paletteKey: 'lesbian' },
            { id: 'dark-lesbian', label: 'Dark Lesbian', type: 'dark', paletteKey: 'darkLesbian' },
        ],
    },
    {
        file: 'MS-DOS.md',
        group: 'vibe',
        meaning:
            'Microsoft DOS — classic PC text mode of the 1980s–90s. Nostalgic terminal energy.',
        body: [
            [
                'MS-DOS',
                extractBlock(readme, '### MS-DOS', '### Matrix II').replace(/^[\s\S]*?\n\n/, ''),
            ],
        ],
        variants: [{ id: 'ms-dos', label: 'MS-DOS', type: 'dark', paletteKey: 'msdos' }],
    },
    {
        file: 'Matrix-II.md',
        group: 'vibe',
        meaning:
            'Reference to The Matrix (1999) — neon green on near-black; cyberpunk terminal.',
        body: [
            [
                'Matrix II',
                extractBlock(readme, '### Matrix II', '### X-Files').replace(/^[\s\S]*?\n\n/, ''),
            ],
        ],
        variants: [{ id: 'matrix-ii', label: 'Matrix II', type: 'dark', paletteKey: 'matrix2' }],
    },
    {
        file: 'X-Files.md',
        group: 'vibe',
        meaning:
            'The X-Files (1993–) — FBI basement files, fluorescent bureaucracy, and night woods.',
        body: [
            ['Light X-Files', extractBlock(readme, '**Light X-Files**', '**Dark X-Files**')],
            ['Dark X-Files', extractBlock(readme, '**Dark X-Files**', '### Only Yesterday')],
        ],
        variants: [
            { id: 'light-x-files', label: 'Light X-Files', type: 'light', paletteKey: 'xfiles' },
            { id: 'dark-x-files', label: 'Dark X-Files', type: 'dark', paletteKey: 'darkXfiles' },
        ],
    },
    {
        file: 'Yesterday.md',
        group: 'vibe',
        meaning:
            'Only Yesterday (Omohide Poro Poro, 1991) — Studio Ghibli memory of rural Yamagata: safflower, rice gold, countryside green.',
        body: [
            [
                'Light Only Yesterday',
                extractBlock(readme, '**Light Only Yesterday**', '**Dark Only Yesterday**'),
            ],
            [
                'Dark Only Yesterday',
                extractBlock(readme, '**Dark Only Yesterday**', '### Country themes'),
            ],
        ],
        variants: [
            {
                id: 'light-only-yesterday',
                label: 'Light Only Yesterday',
                type: 'light',
                paletteKey: 'onlyYesterday',
            },
            {
                id: 'dark-only-yesterday',
                label: 'Dark Only Yesterday',
                type: 'dark',
                paletteKey: 'darkOnlyYesterday',
            },
        ],
    },
];

const countryTable = extractBlock(readme, '| Theme | Light | Dark |', 'Primary hues tint');
const countryRows = [
    ...countryTable.matchAll(/\|\s*\*\*([^*]+)\*\*\s*\|\s*(.+?)\s*\|\s*(.+?)\s*\|/g),
];
const slugOf = {
    Brasil: 'brasil',
    Canada: 'canada',
    China: 'china',
    German: 'german',
    India: 'india',
    Italy: 'italy',
    Japan: 'japan',
    Norway: 'norway',
    Russia: 'russia',
    Taiwan: 'taiwan',
    Thai: 'thai',
    UK: 'uk',
    Ukraine: 'ukraine',
    USA: 'usa',
    Viet: 'viet',
};
const camel = {
    brasil: 'Brasil',
    canada: 'Canada',
    china: 'China',
    german: 'German',
    india: 'India',
    italy: 'Italy',
    japan: 'Japan',
    norway: 'Norway',
    russia: 'Russia',
    taiwan: 'Taiwan',
    thai: 'Thai',
    uk: 'Uk',
    ukraine: 'Ukraine',
    usa: 'Usa',
    viet: 'Viet',
};
for (const [, name, light, dark] of countryRows) {
    const slug = slugOf[name.trim()];
    if (!slug) {
        continue;
    }
    const Cap = name.trim();
    catalogs.push({
        file: `${Cap.replace(/\s+/g, '-')}.md`,
        group: 'country',
        meaning: `Country palette rooted in classic cultural color expression for ${Cap} — landscape, craft, ritual, everyday beauty (not a national flag).`,
        body: [
            [`Country: ${Cap}`, light.replace(/\*\*/g, '').trim()],
            [`Country: ${Cap} (Dark)`, dark.replace(/\*\*/g, '').trim()],
        ],
        variants: [
            {
                id: `country-${slug}`,
                label: `Country: ${Cap}`,
                type: 'light',
                paletteKey: `country${camel[slug]}`,
            },
            {
                id: `dark-country-${slug}`,
                label: `Country: ${Cap} (Dark)`,
                type: 'dark',
                paletteKey: `darkCountry${camel[slug]}`,
            },
        ],
    });
}

const catalogRows = [['id', 'label', 'type', 'group', 'family', 'paletteKey', 'catalog']];
for (const c of catalogs) {
    const lines = [
        `# ${c.file.replace(/\.md$/, '').replace(/-/g, ' ')}`,
        '',
        `group: ${c.group}`,
        '',
        '## Meaning',
        '',
        c.meaning,
        '',
    ];
    for (const [title, text] of c.body) {
        lines.push(
            `## ${title}`,
            '',
            text.replace(/\*\*/g, '').replace(/^—\s*/, '').trim(),
            '',
        );
    }
    lines.push('## Variants', '');
    for (const v of c.variants) {
        lines.push(
            `- \`${v.id}\` — ${v.label} (\`${v.type}\`), paletteKey \`${v.paletteKey}\` in \`${c.group}/\``,
        );
    }
    lines.push('');
    writeFileSync(join(outRoot, 'catalog', c.file), lines.join('\n'));
    const family = c.file.replace(/\.md$/, '');
    for (const v of c.variants) {
        catalogRows.push([
            v.id,
            v.label,
            v.type,
            c.group,
            family,
            v.paletteKey,
            `catalog/${c.file}`,
        ]);
    }
}

writeFileSync(join(outRoot, 'catalog.tsv'), catalogRows.map((r) => r.join('\t')).join('\n') + '\n');

writeFileSync(
    join(outRoot, 'ELEMENTS.md'),
    `# WorldMan theme elements

Color values live in grouped \`.mjs\` palettes (like minor-themes).

\`\`\`text
themes/
  catalog/          # color-language prose per family
  minor/            # soft / pride
  vibe/             # media / retro
  country/          # cultural country palettes
  catalog.tsv
\`\`\`

Each of \`minor/\`, \`vibe/\`, \`country/\`:

| File | Purpose |
|------|---------|
| \`ui-palettes.mjs\` | UiTheme-style roles |
| \`token-palettes.mjs\` | Syntax tokens |
| \`web-palettes.mjs\` | WorldMan web styleclasses (SOP 011) |
| \`erp-palettes.mjs\` | ERP semantic colors |

## Web primaries

background, foreground, primary, primary-foreground, card, card-foreground,
accent, accent-foreground, muted-foreground, border, ring, destructive, success, warning

## ERP primaries

page, page-foreground, panel, panel-foreground, header, toolbar, link,
button, button-foreground, danger, danger-foreground, success, success-foreground,
warning, warning-foreground, table-header, table-border, row-alt, form-border,
form-focus, status-neutral, status-active, status-done, status-error,
amount-in, amount-out
`,
);

writeFileSync(
    join(outRoot, 'SCHEMA.md'),
    `# Theme attachment schema

- \`catalog/*.md\` — family color-language prose
- \`catalog.tsv\` — id, label, type, group, family, paletteKey, catalog
- \`{minor,vibe,country}/{ui,token,web,erp}-palettes.mjs\` — grouped HSL defs

Adapted from minor-themes@${pkg.version}; web + erp are WorldMan additions.
`,
);

writeFileSync(
    join(outRoot, 'README.md'),
    `# WorldMan themes

\`\`\`text
themes/
  catalog/     Yesterday.md  X-Files.md  Innocent.md  …
  minor/       ui|token|web|erp-palettes.mjs
  vibe/        ui|token|web|erp-palettes.mjs
  country/     ui|token|web|erp-palettes.mjs
  catalog.tsv
\`\`\`

- \`000\` → \`$PROJECT/sop/themes/\`
- \`011\` loads **web** palettes (+ mid-tone generation) and the switcher
- ERP may use \`erp-palettes.mjs\`

\`\`\`bash
node tools/extract-worldman-themes.mjs /path/to/minor-themes
\`\`\`

${catalogRows.length - 1} variants indexed in \`catalog.tsv\`.
`,
);

console.log(`wrote themes → ${outRoot} (${catalogRows.length - 1} variants)`);
