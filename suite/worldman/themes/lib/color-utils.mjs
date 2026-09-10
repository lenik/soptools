/**
 * Color utilities for WorldMan theme packs (adapted from minor-themes).
 * Used for discriminability / contrast checks on semantic fg/bg pairs.
 */

export function hexToRgb(hex) {
    const h = hex.replace('#', '');
    return [
        parseInt(h.slice(0, 2), 16),
        parseInt(h.slice(2, 4), 16),
        parseInt(h.slice(4, 6), 16),
    ];
}

export function rgbToHex(r, g, b) {
    return (
        '#' +
        [r, g, b]
            .map((v) => v.toString(16).padStart(2, '0'))
            .join('')
            .toUpperCase()
    );
}

/** @param {number} h @param {number} s @param {number} l  — s,l in 0–100 */
export function hslToHex(h, s, l) {
    h = ((h % 360) + 360) % 360;
    s /= 100;
    l /= 100;
    if (s <= 0) {
        const grey = Math.round(l * 255);
        return rgbToHex(grey, grey, grey);
    }
    const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    const p = 2 * l - q;
    const channel = (offset) => {
        let t = h / 360 + offset;
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1 / 6) return p + (q - p) * 6 * t;
        if (t < 1 / 2) return q;
        if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
        return p;
    };
    return rgbToHex(
        Math.round(channel(1 / 3) * 255),
        Math.round(channel(0) * 255),
        Math.round(channel(-1 / 3) * 255),
    );
}

export function hexToHsl(hex) {
    const [r, g, b] = hexToRgb(hex).map((c) => c / 255);
    const max = Math.max(r, g, b);
    const min = Math.min(r, g, b);
    const l = (max + min) / 2;
    if (max === min) {
        return { h: 0, s: 0, l: l * 100 };
    }
    const d = max - min;
    const s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
    let h;
    switch (max) {
        case r:
            h = ((g - b) / d + (g < b ? 6 : 0)) / 6;
            break;
        case g:
            h = ((b - r) / d + 2) / 6;
            break;
        default:
            h = ((r - g) / d + 4) / 6;
    }
    return { h: h * 360, s: s * 100, l: l * 100 };
}

export function formatHslString(h, s, l) {
    return `hsl(${Math.round(h)}, ${Math.round(s)}%, ${Math.round(l)}%)`;
}

const HSL_RE = /^hsl\(\s*([\d.]+)\s*,\s*([\d.]+)%\s*,\s*([\d.]+)%\s*\)$/i;
const HEX_RE = /^#([0-9A-Fa-f]{6})$/;

export function parseColor(input) {
    const trimmed = String(input).trim();
    const hexMatch = trimmed.match(HEX_RE);
    if (hexMatch) {
        return `#${hexMatch[1].toUpperCase()}`;
    }
    const hslMatch = trimmed.match(HSL_RE);
    if (hslMatch) {
        return hslToHex(Number(hslMatch[1]), Number(hslMatch[2]), Number(hslMatch[3]));
    }
    throw new Error(`Unsupported color format: ${input}`);
}

export function relativeLuminance(rgb) {
    const linear = rgb.map((c) => {
        c /= 255;
        return c <= 0.03928 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4;
    });
    return 0.2126 * linear[0] + 0.7152 * linear[1] + 0.0722 * linear[2];
}

export function contrastRatio(fg, bg) {
    const l1 = relativeLuminance(hexToRgb(parseColor(fg)));
    const l2 = relativeLuminance(hexToRgb(parseColor(bg)));
    return (Math.max(l1, l2) + 0.05) / (Math.min(l1, l2) + 0.05);
}

/**
 * Push fg away from bg until WCAG-style contrast is met (minor-themes method).
 * Direction is chosen by whether white or black reads better on the background
 * (fixes mid-tone brand colors where a naïve bgL&lt;50 lighten fails).
 * @returns {{ hex: string, hsl: string, ratio: number }}
 */
export function ensureContrast(fg, bg, minRatio = 4.5) {
    const bgHex = parseColor(bg);
    const whiteR = contrastRatio('#FFFFFF', bgHex);
    const blackR = contrastRatio('#000000', bgHex);
    const preferLight = whiteR >= blackR;

    let start = parseColor(fg);
    const startR = contrastRatio(start, bgHex);
    if (preferLight && whiteR > startR) {
        start = '#FFFFFF';
    } else if (!preferLight && blackR > startR) {
        start = '#000000';
    }

    let { h, s, l } = hexToHsl(start);

    for (let i = 0; i < 50; i++) {
        const current = hslToHex(h, s, l);
        const ratio = contrastRatio(current, bgHex);
        if (ratio >= minRatio) {
            return { hex: current, hsl: formatHslString(h, s, l), ratio };
        }
        if (preferLight) {
            l = Math.min(96, l + 5);
            s = Math.min(100, Math.max(0, s - 1));
        } else {
            l = Math.max(4, l - 5);
            s = Math.min(100, s + 2);
        }
    }
    const fallback = preferLight ? '#FFFFFF' : '#0A0A0A';
    const { h: fh, s: fs, l: fl } = hexToHsl(fallback);
    return {
        hex: fallback,
        hsl: formatHslString(fh, fs, fl),
        ratio: contrastRatio(fallback, bgHex),
    };
}

/** Semantic fg/bg pairs that must be discriminable. */
export const WEB_FG_BG_PAIRS = [
    ['foreground', 'background'],
    ['card-foreground', 'card'],
    ['primary-foreground', 'primary'],
    ['accent-foreground', 'accent'],
    ['muted-foreground', 'background'],
];

export const ERP_FG_BG_PAIRS = [
    ['page-foreground', 'page'],
    ['panel-foreground', 'panel'],
    ['button-foreground', 'button'],
    ['danger-foreground', 'danger'],
    ['success-foreground', 'success'],
    ['warning-foreground', 'warning'],
];

export const UI_FG_BG_PAIRS = [
    ['windowFg', 'windowBg'],
    ['windowFg', 'surfaceBg'],
    ['listFg', 'windowBg'],
    ['promptFg', 'promptBg'],
    ['footerFg', 'panelBg'],
    ['mutedFg', 'windowBg'],
];

/**
 * Enforce contrast on named pairs; mutates palette in place (HSL strings).
 * If fg alone cannot reach minRatio on a mid-tone fill, nudges the bg
 * lightness so black or white text becomes discriminable.
 * @returns {{ fixed: string[], failed: string[] }}
 */
export function enforcePairs(palette, pairs, minRatio = 4.5) {
    const fixed = [];
    const failed = [];
    for (const [fgKey, bgKey] of pairs) {
        if (!palette[fgKey] || !palette[bgKey]) {
            continue;
        }
        const beforeFg = palette[fgKey];
        const beforeBg = palette[bgKey];
        let { hsl: fgHsl, ratio } = ensureContrast(beforeFg, beforeBg, minRatio);
        let bgHsl = beforeBg;

        if (ratio < minRatio) {
            const bgHex = parseColor(beforeBg);
            const preferLight =
                contrastRatio('#FFFFFF', bgHex) >= contrastRatio('#000000', bgHex);
            let { h, s, l } = hexToHsl(bgHex);
            for (let i = 0; i < 40 && ratio < minRatio; i++) {
                if (preferLight) {
                    l = Math.max(8, l - 4);
                } else {
                    l = Math.min(92, l + 4);
                }
                bgHsl = formatHslString(h, s, l);
                const again = ensureContrast(
                    preferLight ? '#FFFFFF' : '#000000',
                    bgHsl,
                    minRatio,
                );
                fgHsl = again.hsl;
                ratio = again.ratio;
            }
        }

        if (fgHsl !== beforeFg || bgHsl !== beforeBg) {
            palette[fgKey] = fgHsl;
            palette[bgKey] = bgHsl;
            fixed.push(`${fgKey}/${bgKey} → ${ratio.toFixed(2)}`);
        }
        if (ratio < minRatio) {
            failed.push(`${fgKey}/${bgKey} ${ratio.toFixed(2)} < ${minRatio}`);
        }
    }
    return { fixed, failed };
}
