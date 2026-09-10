/**
 * Bilingual catalog prose for WorldMan themes (en + zh_CN).
 * Keys = catalog file stem (Innocent, China, France, …).
 */

/** @typedef {{ group: string, locales?: string[], meaning_en: string, meaning_zh: string, variants: { id: string, label: string, type: string, paletteKey: string, en: string, zh: string }[] }} CatalogFamily */

/** @type {Record<string, CatalogFamily>} */
export const CATALOG = {
    Innocent: {
        group: 'minor',
        meaning_en:
            'Innocent — pure, gentle, unassuming. A blush-and-lavender color language that stays soft without becoming washed-out; intimacy without loudness.',
        meaning_zh:
            'Innocent（纯真）——纯净、柔和、不张扬。以腮红玫瑰与淡紫为语汇，柔而不淡、亲而不躁，避免高饱和喧哗。',
        variants: [
            {
                id: 'innocent',
                label: 'Innocent',
                type: 'light',
                paletteKey: 'innocent',
                en: 'Blush rose and soft lavender (hues ~280–340°). Near-white grounds with a faint pink wash; body text is muted purple-gray. Accents (selection, focus, actions) are dusty rose and periwinkle — airy rather than loud. Prefer calm surfaces; keep decorative chrome quieter than content.',
                zh: '浅色：腮红玫瑰与柔薰衣草（色相约 280–340°）。近白底微粉洗，正文为淡紫灰。选中/焦点/主操作取雾玫瑰与长春花蓝，轻盈而不吵。装饰铬应弱于内容对比。',
            },
            {
                id: 'dark-innocent',
                label: 'Dark Innocent',
                type: 'dark',
                paletteKey: 'darkInnocent',
                en: 'Lavender twilight: soft purple-dusk shell (not neutral gray), lilac-mist text, dusty-rose cursor and periwinkle glow. Innocence becomes a quiet ethereal night — cooler and softer than Dark Maiden’s ink-and-vellum warmth.',
                zh: '深色：薰衣草暮光——柔紫暮色壳层（非中性灰）、丁香雾文字、雾玫瑰光标与长春花辉光。纯真在暗处化为静谧夜色，比 Dark Maiden 的墨色羊皮纸更冷、更柔。',
            },
        ],
    },
    Maiden: {
        group: 'minor',
        meaning_en:
            'Maiden — young woman; also the romantic, delicate sense of “maidenly.” Stationery warmth: soft paper surfaces with readable ink.',
        meaning_zh:
            'Maiden（少女）——年轻女性，亦含「少女气」的浪漫细腻。信笺般的温暖：柔软纸面配可读墨色。',
        variants: [
            {
                id: 'light-maiden',
                label: 'Light Maiden',
                type: 'light',
                paletteKey: 'maiden',
                en: 'Warm shell pink (low-sat red, high lightness) with deep rose-red body text and golden-yellow highlights. Olive-green muted marks add a vintage stationery feel — soft surface, strong readable ink.',
                zh: '浅色：暖贝壳粉（低饱和红、高明度），正文深红玫瑰，高光金黄。橄榄绿弱标记带复古信笺感——面软、墨足、可读。',
            },
            {
                id: 'dark-maiden',
                label: 'Dark Maiden',
                type: 'dark',
                paletteKey: 'darkMaiden',
                en: 'Ink and wax by lamplight: aged vellum umber shadow, iron-gall rose ink text, honey-gold highlights, dried-sage muted marks. Literary, restrained, pre-modern femininity.',
                zh: '深色：灯下墨与蜡——陈年羊皮纸棕影、五倍子玫瑰墨正文、蜜金高光、干鼠尾草绿弱色。文学、克制、前现代的女性气质。',
            },
        ],
    },
    Girl: {
        group: 'minor',
        meaning_en:
            'Girl — youthful, bold, playful (vivid confidence, not childish “girly”). Higher saturation accents on a clean shell.',
        meaning_zh:
            'Girl（女孩）——年轻、大胆、活泼（鲜活自信，而非幼稚「娘气」）。干净底面上使用更高饱和的强调色。',
        variants: [
            {
                id: 'light-girl',
                label: 'Light Girl',
                type: 'light',
                paletteKey: 'girl',
                en: 'High-sat coral-red panels on white, hot-pink quotes, sky-blue intervals. Accents are loud on purpose — full-strength action red, cyan selections, yellow “today” markers.',
                zh: '浅色：白底上高饱和珊瑚红面板、热粉引用、天蓝间隔。强调色故意响亮——主操作大红、青选中、黄「今日」标记。',
            },
            {
                id: 'dark-girl',
                label: 'Dark Girl',
                type: 'dark',
                paletteKey: 'darkGirl',
                en: 'Screen glow in a warm dark room: coral-black shell, loud coral cursor, cyan LED intervals, hot-pink stickers. Youth energy for dim light — not muted into pastel darkness.',
                zh: '深色：暖暗房中的屏幕辉光——珊瑚黑壳、响亮珊瑚光标、青 LED 间隔、热粉贴纸。为暗光保留年轻能量，勿压成粉彩暗调。',
            },
        ],
    },
    Morandi: {
        group: 'minor',
        meaning_en:
            'Named after Giorgio Morandi — dusty, muted, harmonious grays. Gallery-quiet; saturation stays deliberately low.',
        meaning_zh:
            '得名于乔治·莫兰迪——粉尘般、低饱和、和谐的灰调。画廊般安静；饱和度刻意压低。',
        variants: [
            {
                id: 'light-morandi',
                label: 'Light Morandi',
                type: 'light',
                paletteKey: 'morandi',
                en: 'Warm greige window and blue-gray text. Accents are desaturated teal and dusty rose — nothing neon. Borders soft; contrast restrained but still readable.',
                zh: '浅色：暖灰米色窗壳、蓝灰正文。强调为降饱和青绿与雾玫瑰——无霓虹。边框柔和；对比克制但须可读。',
            },
            {
                id: 'dark-morandi',
                label: 'Dark Morandi',
                type: 'dark',
                paletteKey: 'darkMorandi',
                en: 'Studio at closing: umber ochre shadow, chalk-dust cool gray text, dusty teal and faded sage accents — pigments whispering in low north light.',
                zh: '深色：打烊画室——赭棕影、粉笔灰冷色正文、雾青与褪鼠尾草绿强调——北窗低光里的颜料低语。',
            },
        ],
    },
    LGBTQ: {
        group: 'minor',
        meaning_en:
            'Colors from the Gilbert Baker rainbow pride flag as accents — celebratory without painting the whole UI in stripes.',
        meaning_zh:
            '取自吉尔伯特·贝克彩虹旗的色带作强调——欢庆，但不要把整界面刷成彩条。',
        variants: [
            {
                id: 'light-lgbtq',
                label: 'Light LGBTQ',
                type: 'light',
                paletteKey: 'lgbtq',
                en: 'Neutral cool-gray shell; stripe hues as readable accents (red emphasis, orange/yellow highlights, green strings, blue intervals, violet actions, cyan glow).',
                zh: '浅色：中性冷灰壳；彩虹色带作可读强调（红强调、橙/黄高光、绿串、蓝间隔、紫主操作、青辉光）。',
            },
            {
                id: 'dark-lgbtq',
                label: 'Dark LGBTQ',
                type: 'dark',
                paletteKey: 'darkLgbtq',
                en: 'Rain-slick night parade: deep storm-indigo shell so stripe hues glow like wet neon — celebration needs darkness as backdrop, not as mute.',
                zh: '深色：雨夜游行——深风暴靛蓝壳托住彩带，如湿霓虹发光；欢庆需要暗作背景，而非把色彩闷死。',
            },
        ],
    },
    Lesbian: {
        group: 'minor',
        meaning_en:
            'An inward color language — warmth and closeness of love between women; tender, not fragile.',
        meaning_zh:
            '内向的色彩语言——女女之爱的温热与亲近；温柔而非脆弱。',
        variants: [
            {
                id: 'light-lesbian',
                label: 'Light Lesbian',
                type: 'light',
                paletteKey: 'lesbian',
                en: 'Dusk on a shared balcony: warm peach shell, sunset-orange intervals, blush-pink glow, deep rose-magenta keywords and cursor — intimate confidence.',
                zh: '浅色：共处阳台的黄昏——暖桃壳、日落橙间隔、腮红粉辉、深玫品红关键字与光标；亲密而自信。',
            },
            {
                id: 'dark-lesbian',
                label: 'Dark Lesbian',
                type: 'dark',
                paletteKey: 'darkLesbian',
                en: 'Skin warmth after sunset: terracotta umber shadow, ember orange and blush pink on cocoa depth — bodily, quiet confidence; distinct from Maiden’s literary rose.',
                zh: '深色：日落后的体温——陶土棕影、余烬橙与腮红粉落在可可深度上；身体感、安静自信，有别于 Maiden 的文学玫瑰。',
            },
        ],
    },
    'MS-DOS': {
        group: 'vibe',
        meaning_en:
            'Microsoft DOS — classic 1980s–90s PC text mode. Nostalgic terminal energy; not designed for minimal eye strain.',
        meaning_zh:
            'Microsoft DOS——1980–90 年代经典 PC 文本模式。怀旧终端气质；并非为低视疲劳而设计。',
        variants: [
            {
                id: 'ms-dos',
                label: 'MS-DOS',
                type: 'dark',
                paletteKey: 'msdos',
                en: 'Saturated IBM blue background, white foreground, yellow strings, green actions, cyan muted text — the familiar DIR / AUTOEXEC.BAT look. Dark-only.',
                zh: '高饱和 IBM 蓝底、白前景、黄字符串、绿操作、青弱文——熟悉的 DIR / AUTOEXEC.BAT 观感。仅深色。',
            },
        ],
    },
    'Matrix-II': {
        group: 'vibe',
        meaning_en:
            'Reference to The Matrix (1999) — neon green on near-black; cyberpunk terminal best in a dim room.',
        meaning_zh:
            '向《黑客帝国》（1999）致敬——近黑底上的霓虹绿；赛博终端，宜暗室。',
        variants: [
            {
                id: 'matrix-ii',
                label: 'Matrix II',
                type: 'dark',
                paletteKey: 'matrix2',
                en: 'Neon green (~h 135°) on green-tinted near-black panels. Subtle grid/border greens; cursor and active elements glow. Dark-only.',
                zh: '霓虹绿（约 h135°）落在偏绿近黑面板上。网格/边框微绿；光标与激活态发光。仅深色。',
            },
        ],
    },
    'X-Files': {
        group: 'vibe',
        meaning_en:
            'The X-Files — FBI basement files, fluorescent bureaucracy, and night woods. Institutional paper by day; forest CRT by night.',
        meaning_zh:
            '《X 档案》——联邦地下室卷宗、荧光官僚与夜林。日间机构纸色；夜间森林 CRT。',
        variants: [
            {
                id: 'light-x-files',
                label: 'Light X-Files',
                type: 'light',
                paletteKey: 'xfiles',
                en: 'J. Edgar Hoover Building: manila-folder cream shell, institutional gray-green text, FBI forest-green cursor, amber highlighter quotes — case files under strip lighting.',
                zh: '浅色：胡佛大楼——马尼拉文件夹奶油壳、机构灰绿正文、森林绿光标、琥珀荧光笔引用；灯管下的案卷。',
            },
            {
                id: 'dark-x-files',
                label: 'Dark X-Files',
                type: 'dark',
                paletteKey: 'darkXfiles',
                en: 'Basement and pine woods: forest-night green-black shell, CRT phosphor pale text, alien-green cursor, flashlight amber glow, classified red accents.',
                zh: '深色：地下室与松林——森林夜绿黑壳、CRT 磷光淡文、异绿光标、手电琥珀辉、机密红强调。',
            },
        ],
    },
    Yesterday: {
        group: 'vibe',
        meaning_en:
            'Only Yesterday (Omohide Poro Poro, 1991) — Studio Ghibli memory of rural Yamagata: safflower, rice gold, countryside green.',
        meaning_zh:
            '《回忆点点滴滴》（1991）——吉卜力笔下山形乡愁：红花、稻金、田野绿。',
        variants: [
            {
                id: 'light-only-yesterday',
                label: 'Light Only Yesterday',
                type: 'light',
                paletteKey: 'onlyYesterday',
                en: 'Summer memory in sun: sun-washed cream shell, warm brown text, safflower red accents, meadow teal-green intervals, rice-field gold highlights — nostalgic, gentle, sun-faded.',
                zh: '浅色：日光中的夏日记忆——日晒奶油壳、暖棕正文、红花强调、草地青绿间隔、稻田金高光；怀旧、轻柔、晒旧。',
            },
            {
                id: 'dark-only-yesterday',
                label: 'Dark Only Yesterday',
                type: 'dark',
                paletteKey: 'darkOnlyYesterday',
                en: 'Tatami at dusk: umber memory shadow, faded cream text, safflower lantern red, golden recall and evening meadow green — warm memory after sunset.',
                zh: '深色：黄昏榻榻米——赭色记忆影、褪奶油正文、红花灯笼红、金色回想与暮草地绿；日落后的温记忆。',
            },
        ],
    },
};

/** Country families: locales + bilingual cultural briefs. */
export const COUNTRY_CATALOG = {
    Brasil: {
        locales: ['pt_BR'],
        meaning_en:
            'Brasil — tropics of rainforest green, beach gold, and Atlantic blue. Cultural landscape, not the flag.',
        meaning_zh: '巴西——雨林绿、海滩金与大西洋蓝的热带。文化风景，而非国旗。',
        light_en:
            'Lush daylight: pale shell, rainforest greens, beach gold, Atlantic blue edges.',
        light_zh: '茂盛白昼：浅壳、雨林绿、海滩金、大西洋蓝边缘。',
        dark_en:
            'Canopy night: deep green-black shadow, carnival red accents, gold and Atlantic blue as bright edges.',
        dark_zh: '林冠之夜：深绿黑影、嘉年华红强调，金与大西洋蓝作亮边。',
        lightKey: 'countryBrasil',
        darkKey: 'darkCountryBrasil',
        slug: 'brasil',
    },
    Canada: {
        locales: [],
        meaning_en:
            'Canada — north woods: snow, maple autumn crimson, lake and pine. Extended pack (not required by zfr L2).',
        meaning_zh: '加拿大——北方林地：雪、枫红、湖与松。扩展包（非 zfr L2 必选）。',
        light_en: 'Snow white shell, maple autumn crimson actions, lake-and-pine blue-green accents.',
        light_zh: '雪白壳、秋枫绯红主操作、湖松蓝绿强调。',
        dark_en: 'Forest night: pine shadow, maple ember, lake blue — long winter dark with brief warm accents.',
        dark_zh: '林夜：松影、枫火、湖蓝——漫长冬暗中的短暂暖强调。',
        lightKey: 'countryCanada',
        darkKey: 'darkCountryCanada',
        slug: 'canada',
    },
    China: {
        locales: ['zh_CN'],
        meaning_en:
            'China — ink-wash landscape (水墨山水): xuan paper, sumi gray, flower-blue mountains, cinnabar seals. Not a flag palette.',
        meaning_zh:
            '中国——水墨山水：宣纸、焦墨灰、花青远山、朱砂印。非国旗色板。',
        light_en: 'Xuan paper cream, sumi gray text, flower-blue (花青) intervals, cinnabar (朱砂) actions.',
        light_zh: '宣纸奶油、焦墨灰文、花青间隔、朱砂主操作。',
        dark_en: 'Night landscape: deep ink blue-black, pale paper-toned text, cinnabar lantern accents, indigo mist.',
        dark_zh: '夜山水：深墨蓝黑、纸色淡文、朱砂灯笼强调、靛蓝雾。',
        lightKey: 'countryChina',
        darkKey: 'darkCountryChina',
        slug: 'china',
    },
    German: {
        locales: ['de'],
        meaning_en:
            'German / Germany — Rhine workshop craft: mist gray, oak amber, forest green. Gravity and function; not tricolor flag chrome.',
        meaning_zh:
            '德语区 / 德国——莱茵作坊工艺：雾灰、橡木琥珀、森林绿。稳重与功能；非三色旗铬。',
        light_en: 'Mist gray shell, oak amber warmth, forest green accents — craft in daylight.',
        light_zh: '雾灰壳、橡木琥珀暖、森林绿强调——白昼工艺。',
        dark_en: 'Black Forest: green-black shadow, oak warmth, Bauhaus-red accent — gravity and function.',
        dark_zh: '黑森林：绿黑影、橡木暖、包豪斯红强调——重力与功能。',
        lightKey: 'countryGerman',
        darkKey: 'darkCountryGerman',
        slug: 'german',
    },
    India: {
        locales: ['hi'],
        meaning_en:
            'India — temple and market dyes: turmeric, henna, marigold on cream. Ritual color, not the tricolor flag.',
        meaning_zh:
            '印度——庙宇与市集染料：姜黄、指甲花、金盏花落在奶油底。仪式色，非三色旗。',
        light_en: 'Turmeric gold, henna rust, marigold saffron on cream — spice and ritual dye in sun.',
        light_zh: '姜黄金、指甲花锈、金盏红花落奶油——阳光下的香料与仪式染。',
        dark_en: 'Monsoon night: deep indigo shell, turmeric and marigold glow, henna depth between showers.',
        dark_zh: '季风夜：深靛壳、姜黄与金盏辉、阵雨间的指甲花深度。',
        lightKey: 'countryIndia',
        darkKey: 'darkCountryIndia',
        slug: 'india',
    },
    Italy: {
        locales: ['it'],
        meaning_en:
            'Italy — Mediterranean stone: terracotta, olive, marble warm white, coastal azure. Material culture, not the flag.',
        meaning_zh:
            '意大利——地中海石材：陶土、橄榄、大理石暖白、海岸天蓝。物质文化，非国旗。',
        light_en: 'Terracotta and olive on marble-warm white with coastal azure accents.',
        light_zh: '陶土与橄榄落在大理石暖白上，海岸天蓝强调。',
        dark_en: 'Coastal dusk: terracotta shadow, olive depth, azure highlights on warm stone dark.',
        dark_zh: '海岸黄昏：陶土影、橄榄深、暖石暗上的天蓝高光。',
        lightKey: 'countryItaly',
        darkKey: 'darkCountryItaly',
        slug: 'italy',
    },
    Japan: {
        locales: ['ja'],
        meaning_en:
            'Japan — paper and dye: washi, sumi, ai indigo, beni shrine accent, moss, pale sakura wash. Materials and season — not a single emblem.',
        meaning_zh:
            '日本——纸与染：和纸、墨、蓝（蓝染）、红（神社点缀）、苔、淡樱洗。材料与季节——非单一徽标。',
        light_en: 'Warm washi, sumi gray text, ai indigo intervals, beni shrine action, moss and pale sakura wash.',
        light_zh: '暖和纸、墨灰文、蓝间隔、朱红神社主操作、苔与淡樱洗。',
        dark_en: 'Lacquer evening: urushi dark shell, lantern beni, moss garden green, ai shadow.',
        dark_zh: '漆夜：漆黑壳、灯笼朱红、苔园绿、蓝影。',
        lightKey: 'countryJapan',
        darkKey: 'darkCountryJapan',
        slug: 'japan',
    },
    Norway: {
        locales: ['no'],
        meaning_en: 'Norway — fjord and birch; extended pack outside zfr L2.',
        meaning_zh: '挪威——峡湾与桦；zfr L2 外的扩展包。',
        light_en: 'Ice-bright shell, pale sea blue, pine shadow, brief midnight-sun gold.',
        light_zh: '冰亮壳、淡海蓝、松影、短暂午夜阳光金。',
        dark_en: 'Fjord night: deep sea navy, aurora teal, sparse bright edges.',
        dark_zh: '峡湾夜：深海海军蓝、极光青绿、稀疏亮边。',
        lightKey: 'countryNorway',
        darkKey: 'darkCountryNorway',
        slug: 'norway',
    },
    Russia: {
        locales: ['ru'],
        meaning_en:
            'Russia — icons and winter: gilt, liturgical red, birch white, slate blue over snow. Cultural liturgy, not the flag alone.',
        meaning_zh:
            '俄罗斯——圣像与冬天：鎏金、礼仪红、桦白、雪上石板蓝。礼仪文化，非仅国旗。',
        light_en: 'Gilt gold, liturgical red, birch white, slate blue over snow.',
        light_zh: '鎏金、礼仪红、桦白、雪上石板蓝。',
        dark_en: 'Winter twilight: deep slate shell, gilt and red as luminous accents, birch-pale text.',
        dark_zh: '冬暮：深石板壳、鎏金与红作发光强调、桦淡正文。',
        lightKey: 'countryRussia',
        darkKey: 'darkCountryRussia',
        slug: 'russia',
    },
    Taiwan: {
        locales: [],
        meaning_en: 'Taiwan — island mist, tea hills, temple pillars; extended pack.',
        meaning_zh: '台湾——岛雾、茶山、庙柱；扩展包。',
        light_en: 'Mountain-fog gray-blue, tea-hill green, temple-pillar red, Pacific teal.',
        light_zh: '山雾灰蓝、茶山绿、庙柱红、太平洋青绿。',
        dark_en: 'Highland night: misty blue-gray dark, tea-green shadow, temple red and coast teal edges.',
        dark_zh: '高原夜：雾蓝灰暗、茶绿影、庙红与海岸青绿边。',
        lightKey: 'countryTaiwan',
        darkKey: 'darkCountryTaiwan',
        slug: 'taiwan',
    },
    Thai: {
        locales: ['th'],
        meaning_en:
            'Thai — temple and river: stucco cream, saffron robe, lotus pink, jade shade, gold leaf. Ordination gold without gilding the whole shell.',
        meaning_zh:
            '泰国——庙与河：灰泥奶油、僧袍橘黄、莲粉、翡翠荫、金箔。受戒金而不整壳贴金。',
        light_en: 'Warm stucco cream, saffron robe actions, lotus pink glow, jade intervals, gold-leaf accents.',
        light_zh: '暖灰泥奶油、僧袍橘黄主操作、莲粉辉、翡翠间隔、金箔强调。',
        dark_en: 'Temple night: lacquer dark, saffron glow, lotus pink, gold on shadow.',
        dark_zh: '庙夜：漆暗、橘黄辉、莲粉、影上金。',
        lightKey: 'countryThai',
        darkKey: 'darkCountryThai',
        slug: 'thai',
    },
    UK: {
        locales: [],
        meaning_en: 'UK — rain and reading rooms; extended pack.',
        meaning_zh: '英国——雨与书房；扩展包。',
        light_en: 'Fog gray, slate, library leather, muted rose.',
        light_zh: '雾灰、石板、书房皮革、雾玫瑰。',
        dark_en: 'Rainy evening: slate depth, leather-brown warmth, rose in lamplight.',
        dark_zh: '雨夜：石板深、皮革棕暖、灯下玫瑰。',
        lightKey: 'countryUk',
        darkKey: 'darkCountryUk',
        slug: 'uk',
    },
    Ukraine: {
        locales: ['uk'],
        meaning_en: 'Ukraine — sky and wheat (небо й пшениця); cultural field and sky — not flag chrome alone.',
        meaning_zh: '乌克兰——天与麦（небо й пшениця）；田野与天空——非仅国旗铬。',
        light_en: 'Open azure and ripe gold — field meets summer sky.',
        light_zh: '开阔天蓝与熟金——田野遇见夏空。',
        dark_en: 'Evening field: indigo sky dark, wheat shimmer and azure accents.',
        dark_zh: '暮田：靛空暗、麦金微光与天蓝强调。',
        lightKey: 'countryUkraine',
        darkKey: 'darkCountryUkraine',
        slug: 'ukraine',
    },
    USA: {
        locales: [],
        meaning_en: 'USA — open country prairie; extended pack — not bunting/flag chrome.',
        meaning_zh: '美国——开阔草原国度；扩展包——非彩旗/国旗铬。',
        light_en: 'Prairie sky blue, wheat-field amber, barn red, worn denim.',
        light_zh: '草原天蓝、麦田琥珀、谷仓红、旧牛仔蓝。',
        dark_en: 'Prairie dusk: denim-navy shell, barn-red ember, amber horizon.',
        dark_zh: '草原黄昏：牛仔海军壳、谷仓红余烬、琥珀地平线。',
        lightKey: 'countryUsa',
        darkKey: 'darkCountryUsa',
        slug: 'usa',
    },
    Viet: {
        locales: ['vi'],
        meaning_en:
            'Viet — delta and village: rice-paper cream, lacquer red, bamboo green, river blue-gray.',
        meaning_zh: '越南——三角洲与村落：宣纸奶油、漆红、竹绿、河蓝灰。',
        light_en: 'Rice-paper cream, lacquer red actions, bamboo green, river blue-gray intervals.',
        light_zh: '宣纸奶油、漆红主操作、竹绿、河蓝灰间隔。',
        dark_en: 'Lantern dusk: delta indigo dark, lacquer red, bamboo green, warm lantern gold on water.',
        dark_zh: '灯笼黄昏：三角洲靛暗、漆红、竹绿、水上暖灯金。',
        lightKey: 'countryViet',
        darkKey: 'darkCountryViet',
        slug: 'viet',
    },
    France: {
        locales: ['fr'],
        meaning_en:
            'France — limestone towns, vineyard earth, Atlantic slate. Café stone and vineyard green — not bleu-blanc-rouge chrome.',
        meaning_zh:
            '法国——石灰岩小镇、葡萄园土、大西洋石板。咖啡石与葡萄园绿——非蓝白红旗铬。',
        light_en:
            'Limestone cream shell, slate-blue text, vineyard/burgundy actions, Atlantic blue-green accents — daylight café stone.',
        light_zh: '石灰奶油壳、石板蓝文、葡萄园酒红主操作、大西洋蓝绿强调——白昼咖啡馆石色。',
        dark_en:
            'Atlantic night: deep slate shell, limestone-pale text, burgundy ember actions, cool blue accents.',
        dark_zh: '大西洋夜：深石板壳、石灰淡文、酒红余烬主操作、冷蓝强调。',
        lightKey: 'countryFrance',
        darkKey: 'darkCountryFrance',
        slug: 'france',
    },
    Mexico: {
        locales: ['es_MX'],
        meaning_en:
            'Mexico — adobe, agave, highland sky. Clay warmth and desert-green — not the tricolor flag.',
        meaning_zh:
            '墨西哥——土坯、龙舌兰、高原天空。陶土暖与荒漠绿——非三色旗。',
        light_en:
            'Adobe cream shell, earthen brown text, chile-red actions, agave green accents under highland light.',
        light_zh: '土坯奶油壳、泥土棕文、辣椒红主操作、高原光下的龙舌兰绿强调。',
        dark_en:
            'Desert night: warm umber shell, cream text, chile ember, agave glow on dark clay.',
        dark_zh: '荒漠夜：暖赭壳、奶油文、辣椒余烬、暗陶上的龙舌兰辉。',
        lightKey: 'countryMexico',
        darkKey: 'darkCountryMexico',
        slug: 'mexico',
    },
    Korea: {
        locales: ['ko'],
        meaning_en:
            'Korea — hanji paper, celadon glaze, dancheong accents. Court craft and porcelain — not taegeuk flag chrome.',
        meaning_zh:
            '韩国——韩纸、青瓷釉、丹青点缀。宫廷工艺与瓷器——非太极旗铬。',
        light_en:
            'Hanji cream shell, ink-gray text, dancheong red actions, celadon green intervals.',
        light_zh: '韩纸奶油壳、墨灰文、丹青红主操作、青瓷绿间隔。',
        dark_en:
            'Night porcelain: deep ink shell, paper-pale text, dancheong ember, celadon mist accents.',
        dark_zh: '夜瓷器：深墨壳、纸淡文、丹青余烬、青瓷雾强调。',
        lightKey: 'countryKorea',
        darkKey: 'darkCountryKorea',
        slug: 'korea',
    },
    Arab: {
        locales: ['ar'],
        meaning_en:
            'Arab cultural color language — desert sand, copper, indigo night (Maghreb–Mashriq craft). Not a single national flag.',
        meaning_zh:
            '阿拉伯文化色语——沙漠沙、铜、靛蓝夜（马格里布–马什里克工艺）。非单一国旗。',
        light_en:
            'Desert-sand cream shell, copper-umber text, indigo accents, copper actions — souk daylight.',
        light_zh: '沙漠沙奶油壳、铜赭文、靛强调、铜主操作——市集白昼。',
        dark_en:
            'Indigo night: deep indigo shell, sand-pale text, copper lantern actions, cool blue accents.',
        dark_zh: '靛夜：深靛壳、沙淡文、铜灯主操作、冷蓝强调。',
        lightKey: 'countryArab',
        darkKey: 'darkCountryArab',
        slug: 'arab',
    },
    Indonesia: {
        locales: ['id'],
        meaning_en:
            'Indonesia — batik earth, clove spice, tropical canopy green. Archipelago craft — not Merah Putih chrome.',
        meaning_zh:
            '印度尼西亚——蜡染土色、丁香香料、热带林冠绿。群岛工艺——非红白旗铬。',
        light_en:
            'Batik-cream shell, clove-brown text, tropical green accents, warm spice-red actions.',
        light_zh: '蜡染奶油壳、丁香棕文、热带绿强调、暖香料红主操作。',
        dark_en:
            'Canopy night: deep jungle shell, cream text, spice-red ember, tropical green glow.',
        dark_zh: '林冠夜：深丛林壳、奶油文、香料红余烬、热带绿辉。',
        lightKey: 'countryIndonesia',
        darkKey: 'darkCountryIndonesia',
        slug: 'indonesia',
    },
    Netherlands: {
        locales: ['nl'],
        meaning_en:
            'Netherlands — polder sky, brick, canal green. Low-country light — not orange-flag chrome alone.',
        meaning_zh:
            '荷兰——围垦天空、砖、运河绿。低地光线——非仅橙旗铬。',
        light_en:
            'Polder-bright shell, canal slate text, brick-red actions, canal-green accents.',
        light_zh: '围垦亮壳、运河石板文、砖红主操作、运河绿强调。',
        dark_en:
            'Canal night: deep slate shell, brick ember actions, cool canal-green accents.',
        dark_zh: '运河夜：深石板壳、砖余烬主操作、冷运河绿强调。',
        lightKey: 'countryNetherlands',
        darkKey: 'darkCountryNetherlands',
        slug: 'netherlands',
    },
    Poland: {
        locales: ['pl'],
        meaning_en:
            'Poland — birch, Baltic amber, Vistula mist. Northern craft — not simple red-white flag chrome.',
        meaning_zh:
            '波兰——桦、波罗的海琥珀、维斯瓦河雾。北方工艺——非简单红白旗铬。',
        light_en:
            'Birch-bright shell, slate text, amber accents, restrained crimson actions.',
        light_zh: '桦亮壳、石板文、琥珀强调、克制绯红主操作。',
        dark_en:
            'Winter dusk: deep slate shell, amber glow, crimson ember, birch-pale text.',
        dark_zh: '冬暮：深石板壳、琥珀辉、绯红余烬、桦淡正文。',
        lightKey: 'countryPoland',
        darkKey: 'darkCountryPoland',
        slug: 'poland',
    },
    Turkey: {
        locales: ['tr'],
        meaning_en:
            'Turkey — Iznik tile blue, Anatolian earth, Bosphorus. Ceramic and bazaar — not crescent-flag chrome alone.',
        meaning_zh:
            '土耳其——伊兹尼克瓷蓝、安纳托利亚土、博斯普鲁斯。陶瓷与集市——非仅新月旗铬。',
        light_en:
            'Anatolian cream shell, Bosphorus slate text, Iznik blue accents, carmine actions.',
        light_zh: '安纳托利亚奶油壳、博斯普鲁斯石板文、伊兹尼克蓝强调、胭脂红主操作。',
        dark_en:
            'Bosphorus night: deep blue-slate shell, carmine ember, Iznik glow accents.',
        dark_zh: '博斯普鲁斯夜：深蓝石板壳、胭脂余烬、伊兹尼克辉强调。',
        lightKey: 'countryTurkey',
        darkKey: 'darkCountryTurkey',
        slug: 'turkey',
    },
    Sweden: {
        locales: ['sv'],
        meaning_en:
            'Sweden — birch, Baltic ice, falu-red craft accents. Nordic light — not blue-yellow flag chrome.',
        meaning_zh: '瑞典——桦、波罗的海冰、法鲁红工艺强调。北欧光线——非蓝黄旗铬。',
        light_en: 'Ice-bright shell, slate text, falu-red actions, Baltic blue accents.',
        light_zh: '冰亮壳、石板文、法鲁红主操作、波罗的海蓝强调。',
        dark_en: 'Nordic night: deep slate shell, falu ember, cool Baltic accents.',
        dark_zh: '北欧夜：深石板壳、法鲁余烬、冷波罗的海强调。',
        lightKey: 'countrySweden',
        darkKey: 'darkCountrySweden',
        slug: 'sweden',
    },
    Denmark: {
        locales: ['da'],
        meaning_en:
            'Denmark — chalk coast, brick, North Sea. Hygge daylight — not Dannebrog chrome alone.',
        meaning_zh: '丹麦——白垩海岸、砖、北海。Hygge 白昼——非仅国旗十字铬。',
        light_en: 'Chalk cream shell, brick-red actions, North Sea blue accents.',
        light_zh: '白垩奶油壳、砖红主操作、北海蓝强调。',
        dark_en: 'North Sea night: deep slate shell, brick ember, cool sea accents.',
        dark_zh: '北海夜：深石板壳、砖余烬、冷海强调。',
        lightKey: 'countryDenmark',
        darkKey: 'darkCountryDenmark',
        slug: 'denmark',
    },
    Finland: {
        locales: ['fi'],
        meaning_en:
            'Finland — lake ice, pine, midnight-sun gold. Sauna steam light — not flag chrome.',
        meaning_zh: '芬兰——湖冰、松、午夜阳光金。桑拿蒸汽光——非国旗铬。',
        light_en: 'Lake-ice shell, pine-slate text, midnight-sun gold accents, cool lake actions.',
        light_zh: '湖冰壳、松石板文、午夜阳光金强调、冷湖主操作。',
        dark_en: 'Polar dusk: deep lake navy, gold recall, pine-mist accents.',
        dark_zh: '极地黄昏：深湖海军蓝、金色回想、松雾强调。',
        lightKey: 'countryFinland',
        darkKey: 'darkCountryFinland',
        slug: 'finland',
    },
    Czech: {
        locales: ['cs'],
        meaning_en:
            'Czech — Bohemian glass, sandstone, forest. Craft and spa towns — not simple tricolor chrome.',
        meaning_zh: '捷克——波希米亚玻璃、砂岩、森林。工艺与温泉小镇——非简单三色旗铬。',
        light_en: 'Sandstone cream, forest-green accents, restrained crimson actions.',
        light_zh: '砂岩奶油、森林绿强调、克制绯红主操作。',
        dark_en: 'Bohemian night: deep slate shell, forest mist, crimson ember.',
        dark_zh: '波希米亚夜：深石板壳、森林雾、绯红余烬。',
        lightKey: 'countryCzech',
        darkKey: 'darkCountryCzech',
        slug: 'czech',
    },
    Romania: {
        locales: ['ro'],
        meaning_en:
            'Romania — Carpathian green, painted monastery, wheat. Folk craft — not flag chrome alone.',
        meaning_zh: '罗马尼亚——喀尔巴阡绿、彩绘修道院、麦。民间工艺——非仅国旗铬。',
        light_en: 'Wheat cream shell, Carpathian green accents, monastery-red actions.',
        light_zh: '麦奶油壳、喀尔巴阡绿强调、修道院红主操作。',
        dark_en: 'Carpathian night: deep forest shell, wheat shimmer, red ember.',
        dark_zh: '喀尔巴阡夜：深林壳、麦金微光、红余烬。',
        lightKey: 'countryRomania',
        darkKey: 'darkCountryRomania',
        slug: 'romania',
    },
    Greece: {
        locales: ['el'],
        meaning_en:
            'Greece — Aegean, marble, olive. Island light and stone — not blue-white flag chrome alone.',
        meaning_zh: '希腊——爱琴海、大理石、橄榄。岛光与石——非仅蓝白旗铬。',
        light_en: 'Marble white shell, Aegean blue actions, olive green accents.',
        light_zh: '大理石白壳、爱琴蓝主操作、橄榄绿强调。',
        dark_en: 'Aegean night: deep sea shell, marble-pale text, olive and blue glow.',
        dark_zh: '爱琴夜：深海壳、大理石淡文、橄榄与蓝辉。',
        lightKey: 'countryGreece',
        darkKey: 'darkCountryGreece',
        slug: 'greece',
    },
    Hungary: {
        locales: ['hu'],
        meaning_en:
            'Hungary — paprika, Danube, thermal cream. Market spice and river — not tricolor chrome alone.',
        meaning_zh: '匈牙利——辣椒粉、多瑙河、温泉奶油。市集香料与河——非仅三色旗铬。',
        light_en: 'Thermal cream shell, paprika-red actions, Danube blue accents.',
        light_zh: '温泉奶油壳、辣椒红主操作、多瑙蓝强调。',
        dark_en: 'Danube night: warm umber shell, paprika ember, cool river accents.',
        dark_zh: '多瑙夜：暖赭壳、辣椒余烬、冷河强调。',
        lightKey: 'countryHungary',
        darkKey: 'darkCountryHungary',
        slug: 'hungary',
    },
    Bulgaria: {
        locales: ['bg'],
        meaning_en:
            'Bulgaria — rose valley, Balkan green, yogurt cream. Craft and orchard — not flag chrome alone.',
        meaning_zh: '保加利亚——玫瑰谷、巴尔干绿、酸奶奶油。工艺与果园——非仅国旗铬。',
        light_en: 'Yogurt cream shell, rose actions, Balkan green accents.',
        light_zh: '酸奶奶油壳、玫瑰主操作、巴尔干绿强调。',
        dark_en: 'Balkan night: deep green shell, rose glow, cream-pale text.',
        dark_zh: '巴尔干夜：深绿壳、玫瑰辉、奶油淡文。',
        lightKey: 'countryBulgaria',
        darkKey: 'darkCountryBulgaria',
        slug: 'bulgaria',
    },
    Kazakhstan: {
        locales: ['kk'],
        meaning_en:
            'Kazakhstan — steppe gold, high sky, mountain slate. Nomad light — not flag chrome alone.',
        meaning_zh: '哈萨克斯坦——草原金、高天、山石板。游牧光线——非仅国旗铬。',
        light_en: 'Steppe cream shell, sky-blue accents, gold actions.',
        light_zh: '草原奶油壳、天蓝强调、金主操作。',
        dark_en: 'Steppe night: deep slate shell, gold ember, sky accents.',
        dark_zh: '草原夜：深石板壳、金余烬、天强调。',
        lightKey: 'countryKazakhstan',
        darkKey: 'darkCountryKazakhstan',
        slug: 'kazakhstan',
    },
    Philippines: {
        locales: ['fil_PH'],
        meaning_en:
            'Philippines — mango, sea, bamboo. Archipelago daylight — not flag chrome alone.',
        meaning_zh: '菲律宾——芒果、海、竹。群岛白昼——非仅国旗铬。',
        light_en: 'Mango-cream shell, sea-blue accents, warm coral actions, bamboo green.',
        light_zh: '芒果奶油壳、海蓝强调、暖珊瑚主操作、竹绿。',
        dark_en: 'Island night: deep sea shell, mango-pale text, coral ember.',
        dark_zh: '岛夜：深海壳、芒果淡文、珊瑚余烬。',
        lightKey: 'countryPhilippines',
        darkKey: 'darkCountryPhilippines',
        slug: 'philippines',
    },
    Bengal: {
        locales: ['bn'],
        meaning_en:
            'Bengal — jamdani white, monsoon green, vermilion. Delta craft — not flag chrome alone.',
        meaning_zh: '孟加拉——贾木达尼白、季风绿、朱红。三角洲工艺——非仅国旗铬。',
        light_en: 'Jamdani cream shell, monsoon green accents, vermilion actions.',
        light_zh: '贾木达尼奶油壳、季风绿强调、朱红主操作。',
        dark_en: 'Monsoon night: deep green shell, vermilion glow, cream-pale text.',
        dark_zh: '季风夜：深绿壳、朱红辉、奶油淡文。',
        lightKey: 'countryBengal',
        darkKey: 'darkCountryBengal',
        slug: 'bengal',
    },
    Marathi: {
        locales: ['mr'],
        meaning_en:
            'Marathi — Deccan earth, turmeric, peacock. Regional craft for mr locale — distinct from pan-India hi pack.',
        meaning_zh: '马拉地——德干土、姜黄、孔雀。面向 mr 区域工艺——有别于泛印地语 hi 包。',
        light_en: 'Deccan cream shell, turmeric actions, peacock-blue accents.',
        light_zh: '德干奶油壳、姜黄主操作、孔雀蓝强调。',
        dark_en: 'Deccan night: warm earth shell, turmeric ember, peacock glow.',
        dark_zh: '德干夜：暖土壳、姜黄余烬、孔雀辉。',
        lightKey: 'countryMarathi',
        darkKey: 'darkCountryMarathi',
        slug: 'marathi',
    },
    Tamil: {
        locales: ['ta'],
        meaning_en:
            'Tamil — temple gopuram, turmeric, bay. Dravidian craft for ta locale.',
        meaning_zh: '泰米尔——庙塔、姜黄、海湾。面向 ta 的达罗毗荼工艺。',
        light_en: 'Temple cream shell, turmeric-bay warmth, vermilion actions, bay-blue accents.',
        light_zh: '庙奶油壳、姜黄海湾暖、朱红主操作、海湾蓝强调。',
        dark_en: 'Temple night: deep umber shell, vermilion lantern, bay glow.',
        dark_zh: '庙夜：深赭壳、朱红灯笼、海湾辉。',
        lightKey: 'countryTamil',
        darkKey: 'darkCountryTamil',
        slug: 'tamil',
    },
    Telugu: {
        locales: ['te'],
        meaning_en:
            'Telugu — Deccan plateau, mango, indigo. Regional craft for te locale.',
        meaning_zh: '泰卢固——德干高原、芒果、靛蓝。面向 te 的区域工艺。',
        light_en: 'Plateau cream shell, indigo actions, mango-gold accents.',
        light_zh: '高原奶油壳、靛蓝主操作、芒果金强调。',
        dark_en: 'Plateau night: deep indigo shell, mango-pale text, gold ember.',
        dark_zh: '高原夜：深靛壳、芒果淡文、金余烬。',
        lightKey: 'countryTelugu',
        darkKey: 'darkCountryTelugu',
        slug: 'telugu',
    },
    Hebrew: {
        locales: ['he'],
        meaning_en:
            'Hebrew / Israel — Jerusalem stone, olive, Mediterranean. Material culture — not Star-of-David chrome alone.',
        meaning_zh: '希伯来 / 以色列——耶路撒冷石、橄榄、地中海。物质文化——非仅大卫星铬。',
        light_en: 'Stone cream shell, Mediterranean blue actions, olive accents.',
        light_zh: '石奶油壳、地中海蓝主操作、橄榄强调。',
        dark_en: 'Stone night: deep slate shell, olive mist, Mediterranean glow.',
        dark_zh: '石夜：深石板壳、橄榄雾、地中海辉。',
        lightKey: 'countryHebrew',
        darkKey: 'darkCountryHebrew',
        slug: 'hebrew',
    },
    Swahili: {
        locales: ['sw'],
        meaning_en:
            'Swahili — Indian Ocean, baobab, spice coast. East African coastal craft for sw locale.',
        meaning_zh: '斯瓦希里——印度洋、猴面包、香料海岸。面向 sw 的东非海岸工艺。',
        light_en: 'Spice-cream shell, ocean-teal accents, warm spice-red actions.',
        light_zh: '香料奶油壳、海洋青绿强调、暖香料红主操作。',
        dark_en: 'Coast night: deep ocean shell, spice ember, cream-pale text.',
        dark_zh: '海岸夜：深洋壳、香料余烬、奶油淡文。',
        lightKey: 'countrySwahili',
        darkKey: 'darkCountrySwahili',
        slug: 'swahili',
    },
};

