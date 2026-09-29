"use strict";

/* =========================================================
   TIỆM KEM NGỌT NGÀO
   GAME ENGINE
========================================================= */


/* =========================================================
   ICE CREAM DATA
========================================================= */

const ICE_CREAMS = [

    {
        id: "chocolate",
        name: "Kem Sô-cô-la",
        icon: "🍫",
        wholesale: 45,
        price: 90
    },

    {
        id: "strawberry",
        name: "Kem Dâu Tây",
        icon: "🍓",
        wholesale: 50,
        price: 100
    },

    {
        id: "vanilla",
        name: "Kem Vani",
        icon: "🍦",
        wholesale: 35,
        price: 75
    },

    {
        id: "matcha",
        name: "Kem Matcha",
        icon: "🍵",
        wholesale: 55,
        price: 115
    },

    {
        id: "avocado",
        name: "Kem Bơ",
        icon: "🥑",
        wholesale: 60,
        price: 125
    },

    {
        id: "durian",
        name: "Kem Sầu Riêng",
        icon: "🥭",
        wholesale: 75,
        price: 155
    },

    {
        id: "taro",
        name: "Kem Khoai Môn",
        icon: "🍠",
        wholesale: 50,
        price: 105
    },

    {
        id: "oreo",
        name: "Kem Oreo",
        icon: "🍪",
        wholesale: 65,
        price: 130
    },

    {
        id: "yogurt",
        name: "Kem Sữa Chua",
        icon: "🥛",
        wholesale: 45,
        price: 95
    },

    {
        id: "rainbow",
        name: "Kem 7 Màu Rainbow",
        icon: "🌈",
        wholesale: 90,
        price: 180
    },

    {
        id: "coconut",
        name: "Kem Dừa",
        icon: "🥥",
        wholesale: 55,
        price: 110
    },

    {
        id: "coffee",
        name: "Kem Cà Phê",
        icon: "☕",
        wholesale: 60,
        price: 120
    }

];


/* =========================================================
   40+ CUSTOMER NAMES
========================================================= */

const CUSTOMER_NAMES = [

    "Ngọc Anh",
    "Minh Quân",
    "Hà Vy",
    "Hoàng Nam",
    "Tuấn Kiệt",
    "Bảo Anh",
    "Khánh Linh",
    "Đức Anh",
    "Thùy Chi",
    "Quốc Bảo",

    "Phương Linh",
    "Gia Huy",
    "Thanh Trúc",
    "Anh Khoa",
    "Mai Anh",
    "Nhật Minh",
    "Kim Ngân",
    "Thiên An",
    "Minh Thư",
    "Hoàng Long",

    "Yến Nhi",
    "Đăng Khoa",
    "Tú Anh",
    "Thanh Tâm",
    "Quỳnh Như",
    "Hải Đăng",
    "Bích Ngọc",
    "Trung Kiên",
    "Khả Hân",
    "Mạnh Hùng",

    "Lan Chi",
    "Gia Bảo",
    "Thảo Vy",
    "Đình Phong",
    "Mỹ Duyên",
    "Trọng Nghĩa",
    "Hương Giang",
    "Khôi Nguyên",
    "Linh Đan",
    "Duy Khánh",

    "Minh Châu",
    "Hoài Nam",
    "Ngân Hà",
    "Thanh Vy",
    "Đức Minh",
    "Tường Vi",
    "Quang Huy",
    "An Nhiên"
];


/* =========================================================
   30+ GOOD REVIEWS
========================================================= */

const GOOD_REVIEWS = [

    "Kem sô cô la ở đây ngon xỉu luôn, chủ shop bán nhiệt tình! ❤️",

    "10/10 điểm, kem matcha đậm vị béo ngậy!",

    "Tiệm xinh xắn, giao kem nhanh thoăn thoắt ❤️",

    "Kem dâu tươi mát mát lạnh, chắc chắn sẽ quay lại!",

    "Kem vani thơm nhẹ, ăn một lần là ghiền luôn.",

    "Mình cực thích kem Oreo, topping giòn giòn ngon ghê!",

    "Kem sầu riêng thơm béo, đúng gu của mình!",

    "Không gian tiệm dễ thương quá trời luôn 🥰",

    "Nhân viên phục vụ nhanh và vui vẻ.",

    "Kem 7 màu nhìn xinh mà ăn cũng ngon nữa!",

    "Matcha rất thơm, không bị ngọt gắt.",

    "Kem khoai môn béo bùi, ăn cực đã.",

    "Trời nóng mà có ly kem dâu là hết bài!",

    "Shop đóng gói đẹp, phục vụ cũng dễ thương.",

    "Kem sữa chua chua nhẹ, ăn rất mát.",

    "Kem bơ béo béo thơm thơm, ngon bất ngờ!",

    "Mình sẽ rủ bạn bè tới đây ăn tiếp.",

    "Giá ổn, kem ngon, phục vụ nhanh.",

    "Kem cà phê thơm đúng vị mình thích.",

    "Kem dừa rất thơm, vị tự nhiên.",

    "Một trong những tiệm kem mình thích nhất!",

    "Chủ tiệm dễ thương, bán hàng có tâm.",

    "Kem lạnh vừa đủ, không bị cứng.",

    "Ăn xong vẫn muốn gọi thêm một ly nữa 😂",

    "Kem chocolate đậm vị, rất hợp người mê cacao.",

    "Bạn nhân viên phục vụ siêu nhanh!",

    "Tiệm nhỏ nhưng rất sạch sẽ và đáng yêu.",

    "Kem matcha béo thơm, ưng cái bụng!",

    "Dâu tây thơm mùi trái cây thật.",

    "Oreo ăn giòn giòn rất vui miệng.",

    "Kem ngon, phục vụ nhanh, mình cho 5 sao!",

    "Sẽ quay lại thử thêm nhiều vị khác.",

    "Một ly kem làm ngày nóng trở nên dễ chịu hơn.",

    "Tiệm có nhiều vị lạ, rất thú vị.",

    "Kem sầu riêng thơm nức luôn nha!",

    "Vị kem vừa miệng, không quá ngọt.",

    "Đồ ăn ngon mà nhân viên còn nhiệt tình nữa.",

    "Rất thích phong cách pastel của tiệm!",

    "Bạn mình giới thiệu và đúng là ngon thật.",

    "Kem tan vừa phải, ăn rất thích.",

    "Mình đã tìm được tiệm kem ruột rồi!"

];


/* =========================================================
   30+ BAD REVIEWS
========================================================= */

const BAD_REVIEWS = [

    "Bắt khách chờ héo cả người, dẹp tiệm đi! 😡",

    "Thái độ phục vụ chán, hết kem cũng không nói!",

    "Chậm như rùa, cạch mặt shop này luôn!",

    "Đợi lâu quá trời, kem chưa thấy mà muốn tan chảy rồi.",

    "Shop phục vụ lâu quá, mình đi đây!",

    "Nắng muốn xỉu mà đứng chờ mãi không ai bán.",

    "Đợi lâu hơn cả thời gian ăn hết một cây kem!",

    "Shop ơi nhanh lên chứ, mình khát nước luôn rồi!",

    "Chờ tới mức hết kiên nhẫn luôn á!",

    "Đứng đợi hoài mà không được phục vụ.",

    "Kem ngon không biết nhưng chờ thì quá lâu.",

    "Mình bỏ về vì đợi lâu quá.",

    "Shop cần cải thiện tốc độ phục vụ nha.",

    "Đang nóng mà phải chờ lâu thật sự rất mệt.",

    "Không có ai phục vụ thì nói sớm chứ!",

    "Đợi lâu quá nên mình qua tiệm khác.",

    "Chậm quá trời chậm!",

    "Mất kiên nhẫn luôn rồi đó shop!",

    "Đứng chờ tới muốn hóa thành cây kem.",

    "Phục vụ kiểu này khách bỏ đi hết mất.",

    "Mình chờ lâu quá rồi!",

    "Kem chưa thấy đâu mà người đã nóng chảy.",

    "Shop bán hàng hơi chậm nha.",

    "Không thể đứng chờ lâu như vậy được.",

    "Mình bỏ về đây, lần sau tính tiếp.",

    "Thật sự quá lâu để mua một cây kem.",

    "Shop cần thêm người phục vụ rồi.",

    "Chờ lâu làm mình tụt mood luôn.",

    "Đợi mòn dép mới thấy shop!",

    "Mình xin phép đi tìm tiệm khác.",

    "Lâu quá nên không còn muốn ăn kem nữa.",

    "Đợi tới khi kem tan trong tưởng tượng luôn.",

    "Phục vụ chậm quá shop ơi!",

    "Mong lần sau shop phục vụ nhanh hơn.",

    "Đứng đây lâu quá chân mỏi luôn rồi."

];


/* =========================================================
   NATURAL CUSTOMER DIALOGUES
========================================================= */

const CUSTOMER_DIALOGUES = [

    {
        ids: ["chocolate"],
        texts: [
            "Dạ shop bán cho em kem sô cô la với ạ!",
            "Cho em một cây chocolate nha shop!",
            "Shop ơi, em lấy kem chocolate thơm thơm nha!"
        ]
    },

    {
        ids: ["strawberry"],
        texts: [
            "Cho em một kem dâu tây với ạ!",
            "Shop ơi, em muốn ăn kem dâu!",
            "Dạ cho em một cây dâu tây nha!"
        ]
    },

    {
        ids: ["vanilla"],
        texts: [
            "Cho em một kem vani gấp với, nắng quá!",
            "Shop bán em kem vani nha!",
            "Em lấy một cây vani ạ!"
        ]
    },

    {
        ids: ["matcha"],
        texts: [
            "Cho em một kem matcha nha shop!",
            "Dạ em lấy matcha béo béo!",
            "Shop ơi cho em kem matcha với ạ!"
        ]
    },

    {
        ids: ["durian"],
        texts: [
            "Anh lấy một cây kem sầu riêng thơm béo nha em!",
            "Cho anh kem sầu riêng nhé!",
            "Shop có kem sầu riêng không? Cho anh một cây!"
        ]
    },

    {
        ids: ["oreo"],
        texts: [
            "Cho em một kem Oreo với ạ!",
            "Em lấy Oreo nha shop!",
            "Shop cho em kem Oreo giòn giòn!"
        ]
    },

    {
        ids: ["rainbow"],
        texts: [
            "Cho em kem 7 màu nha, nhìn xinh quá!",
            "Em muốn một cây Rainbow!",
            "Shop ơi cho em kem cầu vồng!"
        ]
    }

];


/* =========================================================
   ANGRY SPEECH
========================================================= */

const ANGRY_DIALOGUES = [

    "Shop làm ăn lề mề quá! 😤",

    "Đợi héo cả người rồi! 😭",

    "Ủa shop ngủ quên hả? 😑",

    "Nắng quá mà chờ lâu quá trời!",

    "Chậm như rùa luôn á!",

    "Tôi đứng đây muốn tan chảy rồi!",

    "Nhanh lên thì còn kịp chứ!",

    "Thôi mình đi tiệm khác đây!",

    "Chờ một ly kem mà lâu quá!",

    "Mất kiên nhẫn thật sự luôn!",

    "Shop ơi phục vụ khách đi chứ!",

    "Đứng đợi tới mọc rễ luôn rồi! 🌱"

];


/* =========================================================
   CUSTOMER EMOJIS
========================================================= */

const CUSTOMER_EMOJIS = [
    "😊",
    "🙂",
    "🥰",
    "😎",
    "🤗",
    "😋",
    "😁",
    "☺️"
];


/* =========================================================
   GAME STATE
========================================================= */

const DEFAULT_STATE = {

    money: 2000,

    day: 1,

    hour: 8,

    minute: 0,

    shopOpen: false,

    gameStarted: false,

    dayFinished: false,

    customer: null,

    customerSequence: 0,

    revenueToday: 0,

    tipsToday: 0,

    costsToday: 0,

    customersToday: 0,

    servedToday: 0,

    reviews: [],

    usedReviewTexts: [],

    usedNamesToday: [],

    inventory: {},

    upgrades: {

        counter: 1,

        freezer: 0,

        employee: 0,

        decoration: 0

    }

};


/* =========================================================
   CREATE STATE
========================================================= */

let game = createFreshState();


function createFreshState() {

    const state =
        JSON.parse(
            JSON.stringify(DEFAULT_STATE)
        );

    ICE_CREAMS.forEach(item => {

        state.inventory[item.id] = 2;

    });

    return state;

}


/* =========================================================
   SAVE / LOAD
========================================================= */

const SAVE_KEY =
    "tiem_kem_ngot_ngao_save_v2";


function saveGame() {

    try {

        localStorage.setItem(
            SAVE_KEY,
            JSON.stringify(game)
        );

    } catch (error) {

        console.warn(
            "Không thể lưu game:",
            error
        );

    }

}


function loadGame() {

    try {

        const raw =
            localStorage.getItem(
                SAVE_KEY
            );

        if (!raw) return;

        const saved =
            JSON.parse(raw);

        game = {

            ...createFreshState(),

            ...saved,

            upgrades: {

                ...createFreshState().upgrades,

                ...(saved.upgrades || {})

            },

            inventory: {

                ...createFreshState().inventory,

                ...(saved.inventory || {})

            },

            reviews:
                Array.isArray(saved.reviews)
                ? saved.reviews
                : [],

            usedReviewTexts:
                Array.isArray(saved.usedReviewTexts)
                ? saved.usedReviewTexts
                : [],

            usedNamesToday:
                Array.isArray(saved.usedNamesToday)
                ? saved.usedNamesToday
                : []

        };

    } catch (error) {

        console.warn(
            "Save game bị lỗi. Tạo game mới.",
            error
        );

        game =
            createFreshState();

    }

}


/* =========================================================
   UTILITIES
========================================================= */

function money(value) {

    return Math.round(value)
        .toLocaleString("vi-VN");

}


function randomItem(array) {

    if (!array.length) return null;

    return array[
        Math.floor(
            Math.random() * array.length
        )
    ];

}


function clamp(
    value,
    min,
    max
) {

    return Math.max(
        min,
        Math.min(max, value)
    );

}


function getIceCream(id) {

    return ICE_CREAMS.find(
        item => item.id === id
    );

}


function totalInventory() {

    return Object.values(
        game.inventory
    ).reduce(
        (sum, amount) =>
            sum + Number(amount || 0),
        0
    );

}


function getInventoryCapacity() {

    return (
        30 +

        ((game.upgrades.counter - 1) * 10) +

        (game.upgrades.freezer * 20)
    );

}


function getPriceMultiplier() {

    return 1 +
        game.upgrades.decoration * .05;

}


function getPatienceBonus() {

    return game.upgrades.decoration * 10;

}


/* =========================================================
   TAB SYSTEM
========================================================= */

function switchTab(tab) {

    document
        .querySelectorAll(".page")
        .forEach(page => {

            page.classList.remove(
                "active"
            );

        });

    const selected =
        document.getElementById(
            `tab-${tab}`
        );

    if (selected) {

        selected.classList.add(
            "active"
        );

    }

    document
        .querySelectorAll(".nav-button")
        .forEach(button => {

            button.classList.toggle(
                "active",
                button.dataset.tab === tab
            );

        });

    if (tab === "warehouse") {

        renderWarehouse();

    }

    if (tab === "upgrades") {

        renderUpgrades();

    }

    if (tab === "reviews") {

        renderReviews();

    }

}


/* =========================================================
   INTRO
========================================================= */

function startGame() {

    game.gameStarted = true;

    document
        .getElementById("introModal")
        .classList.remove("show");

    saveGame();

    showDayTransition(() => {

        openShop();

    });

}


/* =========================================================
   DAY TRANSITION
========================================================= */

function showDayTransition(callback) {

    const transition =
        document.getElementById(
            "dayTransition"
        );

    transition.classList.remove(
        "opening"
    );

    transition.classList.add(
        "show"
    );

    setTimeout(() => {

        transition.classList.add(
            "opening"
        );

    }, 650);

    setTimeout(() => {

        transition.classList.remove(
            "show",
            "opening"
        );

        if (callback) {

            callback();

        }

    }, 1500);

}


/* =========================================================
   OPEN SHOP
========================================================= */

function openShop() {

    if (game.dayFinished) {

        return;

    }

    if (game.shopOpen) {

        toast("🍦 Tiệm đang mở rồi!");

        return;

    }

    game.shopOpen = true;

    updateShopButtons();

    updateUI();

    toast(
        "🚪 Tiệm đã mở cửa! Khách sắp tới!"
    );

    setTimeout(() => {

        if (
            game.shopOpen &&
            !game.customer
        ) {

            spawnCustomer();

        }

    }, 1200);

    saveGame();

}


/* =========================================================
   CLOSE SHOP
========================================================= */

function closeShop() {

    if (!game.shopOpen) return;

    game.shopOpen = false;

    if (game.customer) {

        toast(
            "👋 Khách hiện tại sẽ được phục vụ trước khi đóng cửa."
        );

    } else {

        toast(
            "🔒 Tiệm đã đóng cửa."
        );

    }

    updateShopButtons();

    updateUI();

    saveGame();

}


/* =========================================================
   SHOP BUTTONS
========================================================= */

function updateShopButtons() {

    const openButton =
        document.getElementById(
            "openShopButton"
        );

    const closeButton =
        document.getElementById(
            "closeShopButton"
        );

    const serveButton =
        document.getElementById(
            "serveButton"
        );

    openButton.disabled =
        game.shopOpen ||
        game.dayFinished;

    closeButton.disabled =
        !game.shopOpen;

    serveButton.disabled =
        !game.customer ||
        !game.shopOpen ||
        game.dayFinished;

}


/* =========================================================
   WAREHOUSE
========================================================= */

function renderWarehouse() {

    const grid =
        document.getElementById(
            "icecreamGrid"
        );

    if (!grid) return;

    grid.innerHTML = "";

    const capacity =
        getInventoryCapacity();

    const current =
        totalInventory();

    document.getElementById(
        "warehouseCapacity"
    ).textContent =
        `${current} / ${capacity}`;

    ICE_CREAMS.forEach(item => {

        const stock =
            game.inventory[item.id] || 0;

        const sellPrice =
            Math.round(
                item.price *
                getPriceMultiplier()
            );

        const card =
            document.createElement(
                "article"
            );

        card.className =
            "icecream-card";

        card.innerHTML = `

            <div class="icecream-icon">
                ${item.icon}
            </div>

            <div class="icecream-name">
                ${item.name}
            </div>

            <div class="icecream-price">
                Nhập:
                ${money(item.wholesale)}đ
                <br>
                Bán:
                ${money(sellPrice)}đ
            </div>

            <div class="stock-count">
                Trong kho:
                <strong>${stock}</strong>
            </div>

            <button
                class="buy-button"
                onclick="buyIceCream('${item.id}')"
            >
                📦 NHẬP 1 THÙNG
            </button>

        `;

        grid.appendChild(card);

    });

}


/* =========================================================
   BUY ICE CREAM
========================================================= */

function buyIceCream(id) {

    const item =
        getIceCream(id);

    if (!item) return;

    if (
        totalInventory() >=
        getInventoryCapacity()
    ) {

        toast(
            "📦 Kho đầy rồi! Hãy nâng cấp kho."
        );

        return;

    }

    if (game.money < item.wholesale) {

        toast(
            "💰 Không đủ tiền nhập kem!"
        );

        return;

    }

    game.money -=
        item.wholesale;

    game.costsToday +=
        item.wholesale;

    game.inventory[id] =
        (game.inventory[id] || 0) + 1;

    renderWarehouse();

    updateUI();

    saveGame();

    toast(
        `${item.icon} Đã nhập ${item.name}!`
    );

}


/* =========================================================
   UPGRADES
========================================================= */

const UPGRADES = [

    {
        id: "counter",

        icon: "🏪",

        title: "Nâng cấp quầy kem",

        description:
            "Tăng sức chứa kho thêm 10 đơn vị mỗi cấp.",

        max: 5,

        baseCost: 600
    },

    {
        id: "freezer",

        icon: "🧊",

        title: "Tủ đông bảo quản",

        description:
            "Mỗi cấp tăng thêm 20 chỗ chứa kem.",

        max: 5,

        baseCost: 850
    },

    {
        id: "employee",

        icon: "🧑‍🍳",

        title: "Thuê nhân viên",

        description:
            "Nhân viên tự động phục vụ khách. Cấp cao giúp phục vụ nhanh hơn.",

        max: 4,

        baseCost: 1300
    },

    {
        id: "decoration",

        icon: "🌷",

        title: "Trang trí tiệm",

        description:
            "Khách kiên nhẫn hơn và giá bán tăng 5% mỗi cấp.",

        max: 5,

        baseCost: 950
    }

];


function getUpgradeCost(upgrade) {

    const level =
        game.upgrades[
            upgrade.id
        ];

    return Math.round(
        upgrade.baseCost *
        Math.pow(1.65, level)
    );

}


function renderUpgrades() {

    const list =
        document.getElementById(
            "upgradeList"
        );

    if (!list) return;

    list.innerHTML = "";

    UPGRADES.forEach(upgrade => {

        const level =
            game.upgrades[
                upgrade.id
            ];

        const maxed =
            level >= upgrade.max;

        const cost =
            getUpgradeCost(
                upgrade
            );

        const card =
            document.createElement(
                "article"
            );

        card.className =
            "upgrade-card";

        card.innerHTML = `

            <div class="upgrade-icon">
                ${upgrade.icon}
            </div>

            <div>

                <div class="upgrade-title">
                    ${upgrade.title}
                </div>

                <div class="upgrade-description">
                    ${upgrade.description}
                </div>

                <div class="upgrade-footer">

                    <span class="upgrade-level">
                        Cấp ${level}/${upgrade.max}
                    </span>

                    <button
                        class="upgrade-button"
                        ${maxed ? "disabled" : ""}
                        onclick="buyUpgrade('${upgrade.id}')"
                    >
                        ${
                            maxed
                            ? "TỐI ĐA"
                            : `💰 ${money(cost)}đ`
                        }
                    </button>

                </div>

            </div>
        `;

        list.appendChild(card);

    });

}


/* =========================================================
   BUY UPGRADE
========================================================= */

function buyUpgrade(id) {

    const upgrade =
        UPGRADES.find(
            item => item.id === id
        );

    if (!upgrade) return;

    const level =
        game.upgrades[id];

    if (level >= upgrade.max) {

        toast(
            "⭐ Nâng cấp đã đạt tối đa!"
        );

        return;

    }

    const cost =
        getUpgradeCost(upgrade);

    if (game.money < cost) {

        toast(
            "💰 Chưa đủ tiền nâng cấp!"
        );

        return;

    }

    game.money -= cost;

    game.costsToday += cost;

    game.upgrades[id]++;

    renderUpgrades();

    renderWarehouse();

    updateUI();

    saveGame();

    toast(
        `${upgrade.icon} Đã nâng cấp ${upgrade.title}!`
    );

}


/* =========================================================
   FIND UNUSED NAME
========================================================= */

function getUniqueCustomerName() {

    let available =
        CUSTOMER_NAMES.filter(
            name =>
                !game.usedNamesToday.includes(
                    name
                )
        );

    if (!available.length) {

        game.usedNamesToday = [];

        available =
            [...CUSTOMER_NAMES];

    }

    const name =
        randomItem(available);

    game.usedNamesToday.push(
        name
    );

    return name;

}


/* =========================================================
   FIND UNIQUE REVIEW TEXT
========================================================= */

function getUniqueReviewText(
    pool
) {

    let available =
        pool.filter(
            text =>
                !game.usedReviewTexts.includes(
                    text
                )
        );

    if (!available.length) {

        /*
           Không reset toàn bộ lịch sử ngay.
           Chỉ cho phép dùng lại sau khi
           toàn bộ câu trong pool đã được sử dụng.
        */

        game.usedReviewTexts =
            game.usedReviewTexts.filter(
                text =>
                    !pool.includes(text)
            );

        available =
            pool.filter(
                text =>
                    !game.usedReviewTexts.includes(
                        text
                    )
            );

    }

    const text =
        randomItem(available);

    game.usedReviewTexts.push(
        text
    );

    /*
       Giữ lịch sử vừa đủ để save không phình.
    */

    if (
        game.usedReviewTexts.length > 120
    ) {

        game.usedReviewTexts =
            game.usedReviewTexts.slice(
                -100
            );

    }

    return text;

}


/* =========================================================
   CUSTOMER ORDER
========================================================= */

function createCustomerOrder() {

    const available =
        ICE_CREAMS.filter(
            item =>
                (game.inventory[item.id] || 0)
                > 0
        );

    if (!available.length) {

        return null;

    }

    /*
       Một số khách gọi 2 món.
    */

    const first =
        randomItem(available);

    let second = null;

    if (
        available.length > 1 &&
        Math.random() < .35
    ) {

        const possible =
            available.filter(
                item =>
                    item.id !== first.id
            );

        second =
            randomItem(possible);

    }

    const items = [

        {
            id: first.id,
            quantity: 1
        }

    ];

    if (second) {

        items.push({

            id: second.id,
            quantity: 1

        });

    }

    return items;

}


/* =========================================================
   NATURAL DIALOGUE
========================================================= */

function makeCustomerDialogue(
    order
) {

    /*
       Tìm mẫu hội thoại nếu có.
    */

    const first =
        order[0];

    const predefined =
        CUSTOMER_DIALOGUES.filter(
            group =>
                group.ids.includes(
                    first.id
                )
        );

    if (
        predefined.length &&
        Math.random() < .75
    ) {

        return randomItem(
            randomItem(predefined).texts
        );

    }

    const item1 =
        getIceCream(
            order[0].id
        );

    if (
        order.length > 1
    ) {

        const item2 =
            getIceCream(
                order[1].id
            );

        const combo = [

            `Cho chị 1 ${item1.name.replace("Kem ", "")} với 1 ${item2.name.replace("Kem ", "")} nhen!`,

            `Shop ơi cho em ${item1.name} và ${item2.name} nha!`,

            `Dạ cho em hai vị ${item1.name.replace("Kem ", "")} với ${item2.name.replace("Kem ", "")} ạ!`

        ];

        return randomItem(combo);

    }

    const generic = [

        `Dạ shop bán cho em ${item1.name.toLowerCase()} với ạ!`,

        `Cho mình một ${item1.name.toLowerCase()} nha shop!`,

        `Shop ơi cho em ${item1.name.toLowerCase()} với!`,

        `Em lấy một ${item1.name.toLowerCase()} nha!`

    ];

    return randomItem(generic);

}


/* =========================================================
   CUSTOMER SPAWN
========================================================= */

function spawnCustomer() {

    if (!game.shopOpen) return;

    if (game.dayFinished) return;

    if (game.customer) return;

    if (game.hour >= 20) return;

    const order =
        createCustomerOrder();

    if (!order) {

        toast(
            "📦 Hết kem rồi! Vào KHO HÀNG nhập thêm."
        );

        return;

    }

    const patience =
        55 +
        getPatienceBonus();

    const customer = {

        id:
            ++game.customerSequence,

        name:
            getUniqueCustomerName(),

        avatar:
            randomItem(
                CUSTOMER_EMOJIS
            ),

        order,

        dialogue:
            makeCustomerDialogue(
                order
            ),

        patience,

        maxPatience:
            patience,

        state: "entering",

        gender:
            Math.random() < .5
            ? "male"
            : "female"

    };

    game.customer =
        customer;

    game.customersToday++;

    renderCustomer();

    updateCurrentOrder();

    updateShopButtons();

    updateUI();

}


/* =========================================================
   CHARACTER SVG
========================================================= */

function characterSVG(
    gender
) {

    const isFemale =
        gender === "female";

    const hair =
        isFemale
        ? `
            <path
                class="hair"
                d="
                M26 39
                C17 15 34 4 51 8
                C70 5 86 20 78 45
                L77 72
                C72 79 65 82 60 78
                L62 54
                C72 34 64 20 51 20
                C36 20 31 30 31 48
                L27 70
                C20 62 20 48 26 39
                Z"
                fill="#704b43"
            />
        `
        :
        `
            <path
                class="hair"
                d="
                M27 38
                C27 16 40 8 53 8
                C69 8 80 19 78 38
                L68 31
                L63 20
                L55 29
                L47 19
                L38 31
                L27 38
                Z"
                fill="#57433f"
            />
        `;

    const shirt =
        isFemale
        ? "#eaa3a9"
        : "#8eb8d1";

    return `

        <svg
            viewBox="0 0 100 125"
            xmlns="http://www.w3.org/2000/svg"
        >

            <!-- HAIR BACK -->
            ${hair}

            <!-- NECK -->
            <path
                d="
                M43 64
                L43 76
                L57 76
                L57 64
                Z"
                fill="#efb894"
            />

            <!-- BODY -->
            <path
                d="
                M31 78
                C35 72 42 70 50 70
                C58 70 66 72 70 78
                L77 119
                L23 119
                Z"
                fill="${shirt}"
                stroke="#b98283"
                stroke-width="2"
            />

            <!-- SHIRT DETAIL -->
            <path
                d="
                M44 74
                L50 82
                L56 74"
                fill="none"
                stroke="#ffffff"
                stroke-width="2"
            />

            <!-- ARM LEFT -->
            <path
                d="
                M31 82
                C22 91 20 101 25 106"
                fill="none"
                stroke="${shirt}"
                stroke-width="10"
                stroke-linecap="round"
            />

            <!-- ARM RIGHT -->
            <path
                d="
                M69 82
                C78 91 80 101 75 106"
                fill="none"
                stroke="${shirt}"
                stroke-width="10"
                stroke-linecap="round"
            />

            <!-- FACE -->
            <ellipse
                class="skin"
                cx="50"
                cy="45"
                rx="25"
                ry="27"
                fill="#efb894"
            />

            <!-- HAIR FRONT -->
            ${isFemale ? `
                <path
                    class="hair"
                    d="
                    M26 39
                    C27 16 42 8 54 10
                    C70 11 79 22 77 39
                    C69 28 62 25 53 26
                    C43 27 35 34 26 39
                    Z"
                    fill="#704b43"
                />
            ` : `
                <path
                    class="hair"
                    d="
                    M27 37
                    C29 18 41 10 53 11
                    C66 11 76 19 77 34
                    L65 27
                    L58 20
                    L51 27
                    L44 20
                    L35 31
                    Z"
                    fill="#57433f"
                />
            `}

            <!-- EAR -->
            <circle
                cx="25"
                cy="48"
                r="5"
                fill="#efb894"
            />

            <circle
                cx="75"
                cy="48"
                r="5"
                fill="#efb894"
            />

            <!-- BLUSH -->
            <ellipse
                cx="35"
                cy="53"
                rx="5"
                ry="3"
                fill="#e99692"
                opacity=".65"
            />

            <ellipse
                cx="65"
                cy="53"
                rx="5"
                ry="3"
                fill="#e99692"
                opacity=".65"
            />

            <!-- EYES -->
            <ellipse
                class="eye"
                cx="41"
                cy="45"
                rx="2.7"
                ry="4"
                fill="#463b38"
            />

            <ellipse
                class="eye"
                cx="59"
                cy="45"
                rx="2.7"
                ry="4"
                fill="#463b38"
            />

            <!-- EYEBROWS -->
            <path
                d="M36 38 Q41 35 46 38"
                fill="none"
                stroke="#5c423d"
                stroke-width="2"
                stroke-linecap="round"
            />

            <path
                d="M54 38 Q59 35 64 38"
                fill="none"
                stroke="#5c423d"
                stroke-width="2"
                stroke-linecap="round"
            />

            <!-- NOSE -->
            <path
                d="
                M49 45
                L47 51
                L51 52"
                fill="none"
                stroke="#bf806c"
                stroke-width="1.5"
            />

            <!-- MOUTH -->
            <path
                class="mouth"
                d="M43 58 Q50 63 57 58"
                fill="none"
                stroke="#8d514f"
                stroke-width="2"
                stroke-linecap="round"
            />

            <!-- ANGRY MARK -->
            <path
                class="angry-mark"
                d="
                M78 23
                L86 15
                M84 24
                L89 27"
                fill="none"
                stroke="#e34f4f"
                stroke-width="3"
                stroke-linecap="round"
            />

        </svg>

    `;

}


/* =========================================================
   RENDER CUSTOMER
========================================================= */

function renderCustomer() {

    const area =
        document.getElementById(
            "customerArea"
        );

    area.innerHTML = "";

    if (!game.customer) {

        updateCurrentOrder();

        return;

    }

    const customer =
        game.customer;

    const orderText =
        customer.order
            .map(orderItem => {

                const item =
                    getIceCream(
                        orderItem.id
                    );

                return `${item.icon} ${item.name}`;

            })
            .join(" + ");

    const customerEl =
        document.createElement(
            "div"
        );

    customerEl.className =
        `customer ${customer.state === "angry" ? "angry" : ""}`;

    customerEl.innerHTML = `

        <div class="speech-bubble">
            ${customer.avatar}
            ${customer.dialogue}
        </div>

        <div class="patience-wrap">

            <div class="patience-label">
                ❤️ Kiên nhẫn
            </div>

            <div class="patience-track">

                <div
                    id="customerPatienceBar"
                    class="patience-bar"
                ></div>

            </div>

        </div>

        <div class="character ${
            customer.state === "entering"
            ? "walking"
            : ""
        }">

            ${characterSVG(
                customer.gender
            )}

        </div>

    `;

    area.appendChild(
        customerEl
    );

    requestAnimationFrame(() => {

        customer.state =
            "waiting";

        customerEl.classList.remove(
            "entering"
        );

        customerEl.classList.add(
            "waiting"
        );

        const character =
            customerEl.querySelector(
                ".character"
            );

        if (character) {

            character.classList.remove(
                "walking"
            );

        }

    });

    updatePatience();

}


/* =========================================================
   CURRENT ORDER
========================================================= */

function updateCurrentOrder() {

    const container =
        document.getElementById(
            "currentOrder"
        );

    if (!container) return;

    if (!game.customer) {

        container.innerHTML =
            "Chưa có khách đang chờ.";

        return;

    }

    const customer =
        game.customer;

    const orderText =
        customer.order
            .map(orderItem => {

                const item =
                    getIceCream(
                        orderItem.id
                    );

                return `
                    ${item.icon}
                    <strong>
                        ${item.name}
                    </strong>
                `;

            })
            .join("<br>");

    container.innerHTML = `

        <strong>
            👤 ${customer.name}
        </strong>

        <br>

        ${orderText}

        <br>

        <span style="color:#9a8980;">
            Hãy phục vụ trước khi thanh
            kiên nhẫn về 0!
        </span>

    `;

}


/* =========================================================
   PATIENCE UPDATE
========================================================= */

function updatePatience() {

    if (!game.customer) return;

    const bar =
        document.getElementById(
            "customerPatienceBar"
        );

    if (!bar) return;

    const percentage =
        clamp(
            (
                game.customer.patience /
                game.customer.maxPatience
            ) * 100,
            0,
            100
        );

    bar.style.width =
        `${percentage}%`;

    if (percentage <= 25) {

        bar.style.background =
            "#d97872";

    } else if (percentage <= 50) {

        bar.style.background =
            "#e3b25e";

    } else {

        bar.style.background =
            "#8fc99a";

    }

}


/* =========================================================
   SERVE CUSTOMER
========================================================= */

function serveCustomer() {

    if (!game.customer) {

        toast(
            "👀 Chưa có khách nào đang chờ."
        );

        return;

    }

    if (!game.shopOpen) {

        toast(
            "🔒 Hãy mở cửa tiệm trước!"
        );

        return;

    }

    const customer =
        game.customer;

    /*
       Kiểm tra đủ tất cả món.
    */

    const canServe =
        customer.order.every(
            orderItem =>
                (
                    game.inventory[
                        orderItem.id
                    ] || 0
                ) >=
                orderItem.quantity
        );

    if (!canServe) {

        toast(
            "📦 Không đủ một trong các vị kem khách gọi!"
        );

        return;

    }

    let revenue = 0;

    customer.order.forEach(
        orderItem => {

            const item =
                getIceCream(
                    orderItem.id
                );

            game.inventory[
                orderItem.id
            ] -= orderItem.quantity;

            revenue +=
                item.price *
                orderItem.quantity;

        }
    );

    revenue =
        Math.round(
            revenue *
            getPriceMultiplier()
        );

    const patiencePercent =
        customer.patience /
        customer.maxPatience;

    let tipRate = 0;

    if (patiencePercent >= .75) {

        tipRate = .20;

    } else if (
        patiencePercent >= .50
    ) {

        tipRate = .10;

    } else if (
        patiencePercent >= .25
    ) {

        tipRate = .03;

    }

    const tip =
        Math.round(
            revenue * tipRate
        );

    const total =
        revenue + tip;

    game.money += total;

    game.revenueToday += revenue;

    game.tipsToday += tip;

    game.servedToday++;

    addReview(
        5,
        customer.name,
        customer.avatar,
        getUniqueReviewText(
            GOOD_REVIEWS
        )
    );

    const customerEl =
        document.querySelector(
            ".customer"
        );

    if (customerEl) {

        customerEl.classList.add(
            "leaving"
        );

    }

    game.customer = null;

    updateCurrentOrder();

    updateShopButtons();

    renderReviews();

    renderWarehouse();

    updateUI();

    saveGame();

    toast(
        `🍨 Phục vụ thành công +${money(total)}đ`
        +
        (
            tip > 0
            ? ` · 🎁 Tip ${money(tip)}đ`
            : ""
        )
    );

    /*
       Cho khách mới sau một khoảng nghỉ.
    */

    setTimeout(() => {

        if (
            game.shopOpen &&
            !game.customer &&
            !game.dayFinished
        ) {

            spawnCustomer();

        }

    }, 1600);

}


/* =========================================================
   ANGRY CUSTOMER
========================================================= */

function angryCustomerLeaves() {

    if (!game.customer) return;

    const customer =
        game.customer;

    const customerEl =
        document.querySelector(
            ".customer"
        );

    if (customerEl) {

        customerEl.classList.add(
            "angry"
        );

        customerEl.classList.add(
            "leaving"
        );

        const bubble =
            customerEl.querySelector(
                ".speech-bubble"
            );

        if (bubble) {

            bubble.textContent =
                randomItem(
                    ANGRY_DIALOGUES
                );

        }

    }

    const angryText =
        getUniqueReviewText(
            BAD_REVIEWS
        );

    addReview(
        1,
        customer.name,
        "😡",
        angryText
    );

    toast(
        `😡 ${customer.name}: ${angryText}`
    );

    game.customer = null;

    updateCurrentOrder();

    updateShopButtons();

    renderReviews();

    saveGame();

    setTimeout(() => {

        if (
            game.shopOpen &&
            !game.customer &&
            !game.dayFinished
        ) {

            spawnCustomer();

        }

    }, 1800);

}


/* =========================================================
   ADD REVIEW
========================================================= */

function addReview(
    stars,
    name,
    avatar,
    text
) {

    game.reviews.unshift({

        stars,

        name,

        avatar,

        text,

        time:
            `Ngày ${game.day} · ${formatTime()}`

    });

    /*
       Không để save phình quá lớn.
    */

    if (
        game.reviews.length > 100
    ) {

        game.reviews =
            game.reviews.slice(
                0,
                100
            );

    }

}


/* =========================================================
   RATING
========================================================= */

function getAverageRating() {

    if (!game.reviews.length) {

        return 5;

    }

    const total =
        game.reviews.reduce(
            (sum, review) =>
                sum + review.stars,
            0
        );

    return total /
        game.reviews.length;

}


/* =========================================================
   RENDER REVIEWS
========================================================= */

function renderReviews() {

    const list =
        document.getElementById(
            "reviewList"
        );

    if (!list) return;

    const average =
        getAverageRating();

    document.getElementById(
        "ratingNumber"
    ).textContent =
        average.toFixed(1);

    document.getElementById(
        "reviewCount"
    ).textContent =
        `${game.reviews.length} đánh giá`;

    const rounded =
        Math.round(average);

    document.getElementById(
        "ratingStars"
    ).textContent =
        "★".repeat(
            rounded
        ) +
        "☆".repeat(
            5 - rounded
        );

    list.innerHTML = "";

    if (!game.reviews.length) {

        list.innerHTML = `

            <div class="review">

                <div
                    style="
                        text-align:center;
                        color:#998980;
                        font-size:12px;
                    "
                >
                    💬 Chưa có đánh giá.
                    <br>
                    Hãy phục vụ khách để
                    nhận đánh giá đầu tiên!
                </div>

            </div>

        `;

        return;

    }

    game.reviews.forEach(
        review => {

            const element =
                document.createElement(
                    "article"
                );

            element.className =
                "review";

            element.innerHTML = `

                <div class="review-header">

                    <div class="review-avatar">
                        ${review.avatar}
                    </div>

                    <div class="review-user">

                        <div class="review-name">
                            ${escapeHTML(
                                review.name
                            )}
                        </div>

                        <div class="review-time">
                            ${escapeHTML(
                                review.time
                            )}
                        </div>

                    </div>

                    <div class="review-stars">

                        ${
                            "★".repeat(
                                review.stars
                            )
                        }${
                            "☆".repeat(
                                5 - review.stars
                            )
                        }

                    </div>

                </div>

                <div class="review-text">

                    ${escapeHTML(
                        review.text
                    )}

                </div>

            `;

            list.appendChild(
                element
            );

        }
    );

}


/* =========================================================
   ESCAPE HTML
========================================================= */

function escapeHTML(value) {

    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");

}


/* =========================================================
   TIME
========================================================= */

function formatTime() {

    const h =
        String(game.hour)
            .padStart(2, "0");

    const m =
        String(game.minute)
            .padStart(2, "0");

    return `${h}:${m}`;

}


/* =========================================================
   GAME TIME
========================================================= */

let lastRealSecond =
    Date.now();

let customerSpawnCounter =
    0;


function updateGameClock() {

    if (!game.gameStarted) return;

    if (game.dayFinished) return;

    const now =
        Date.now();

    if (
        now - lastRealSecond <
        1000
    ) {

        return;

    }

    lastRealSecond =
        now;

    /*
       1 giây thật = 1 phút trong game.
    */

    game.minute++;

    if (game.minute >= 60) {

        game.minute = 0;

        game.hour++;

    }

    if (
        game.hour >= 20
    ) {

        finishDay();

        return;

    }

    updateUI();

}


/* =========================================================
   CUSTOMER TIMER
========================================================= */

function updateCustomerLogic() {

    if (!game.gameStarted) return;

    if (game.dayFinished) return;

    if (!game.shopOpen) return;

    /*
       Nhân viên tự động phục vụ.
    */

    if (
        game.customer &&
        game.upgrades.employee > 0
    ) {

        const employeeSpeed =
            0.035 *
            game.upgrades.employee;

        if (
            Math.random() <
            employeeSpeed
        ) {

            serveCustomer();

            return;

        }

    }

    /*
       Khách mất kiên nhẫn.
    */

    if (game.customer) {

        /*
           Decoration càng cao,
           tốc độ tụt kiên nhẫn càng thấp.
        */

        const decrease =
            .55 -
            (
                game.upgrades.decoration *
                .06
            );

        game.customer.patience =
            Math.max(
                0,
                game.customer.patience -
                Math.max(
                    .15,
                    decrease
                )
            );

        updatePatience();

        if (
            game.customer.patience <= 0
        ) {

            angryCustomerLeaves();

        }

    }

}


/* =========================================================
   SPAWN LOGIC
========================================================= */

function updateCustomerSpawner() {

    if (!game.shopOpen) return;

    if (game.dayFinished) return;

    if (game.customer) return;

    if (game.hour >= 20) return;

    customerSpawnCounter++;

    /*
       Khách mặc định khoảng 5-7 giây.
       Trang trí giúp giảm thời gian.
    */

    const target =
        Math.max(
            3,
            7 -
            game.upgrades.decoration
        );

    if (
        customerSpawnCounter >=
        target
    ) {

        customerSpawnCounter = 0;

        if (
            Math.random() < .82
        ) {

            spawnCustomer();

        }

    }

}


/* =========================================================
   DAY END
========================================================= */

function finishDay() {

    if (game.dayFinished) return;

    game.dayFinished = true;

    game.shopOpen = false;

    /*
       Chi phí vận hành.
    */

    const operatingCost =
        100 +
        (
            game.upgrades.employee *
            60
        );

    game.costsToday +=
        operatingCost;

    const profit =
        game.revenueToday +
        game.tipsToday -
        game.costsToday;

    document.getElementById(
        "summaryRevenue"
    ).textContent =
        `${money(
            game.revenueToday
        )}đ`;

    document.getElementById(
        "summaryCost"
    ).textContent =
        `${money(
            game.costsToday
        )}đ`;

    document.getElementById(
        "summaryTips"
    ).textContent =
        `${money(
            game.tipsToday
        )}đ`;

    document.getElementById(
        "summaryCustomers"
    ).textContent =
        game.servedToday;

    const profitElement =
        document.getElementById(
            "summaryProfit"
        );

    profitElement.textContent =
        `${profit >= 0 ? "+" : ""}${money(profit)}đ`;

    profitElement.style.color =
        profit >= 0
        ? "#5d8b64"
        : "#bd625a";

    document
        .getElementById(
            "summaryModal"
        )
        .classList.add("show");

    updateShopButtons();

    updateUI();

    saveGame();

}


/* =========================================================
   NEXT DAY
========================================================= */

function startNextDay() {

    document
        .getElementById(
            "summaryModal"
        )
        .classList.remove("show");

    game.day++;

    game.hour = 8;

    game.minute = 0;

    game.shopOpen = false;

    game.dayFinished = false;

    game.customer = null;

    game.revenueToday = 0;

    game.tipsToday = 0;

    game.costsToday = 0;

    game.customersToday = 0;

    game.servedToday = 0;

    game.usedNamesToday = [];

    customerSpawnCounter = 0;

    updateCustomerDOM();

    updateUI();

    updateShopButtons();

    saveGame();

    showDayTransition(() => {

        openShop();

    });

}


/* =========================================================
   UPDATE CUSTOMER DOM
========================================================= */

function updateCustomerDOM() {

    const area =
        document.getElementById(
            "customerArea"
        );

    if (!game.customer) {

        area.innerHTML = "";

        return;

    }

    renderCustomer();

}


/* =========================================================
   UI UPDATE
========================================================= */

function updateUI() {

    const moneyElement =
        document.getElementById(
            "money"
        );

    if (moneyElement) {

        moneyElement.textContent =
            money(game.money);

    }

    const time =
        formatTime();

    const dayText =
        document.getElementById(
            "dayTimeText"
        );

    if (dayText) {

        dayText.textContent =
            `Ngày ${game.day} · ${time}`;

    }

    const clock =
        document.getElementById(
            "gameClock"
        );

    if (clock) {

        clock.textContent =
            time;

    }

    const badge =
        document.getElementById(
            "clockBadge"
        );

    if (badge) {

        badge.textContent =
            time;

    }

    const revenue =
        document.getElementById(
            "todayRevenue"
        );

    if (revenue) {

        revenue.textContent =
            `${money(
                game.revenueToday +
                game.tipsToday
            )}đ`;

    }

    const customers =
        document.getElementById(
            "todayCustomers"
        );

    if (customers) {

        customers.textContent =
            game.customersToday;

    }

    const average =
        document.getElementById(
            "averageRating"
        );

    if (average) {

        average.textContent =
            getAverageRating()
                .toFixed(1);

    }

    const status =
        document.getElementById(
            "shopStatusText"
        );

    if (status) {

        if (game.dayFinished) {

            status.textContent =
                "Ngày đã kết thúc";

        } else if (game.shopOpen) {

            status.textContent =
                "🟢 Tiệm đang mở cửa";

        } else {

            status.textContent =
                "🔴 Tiệm đang đóng cửa";

        }

    }

    updateShopButtons();

}


/* =========================================================
   TOAST
========================================================= */

function toast(message) {

    const container =
        document.getElementById(
            "toastContainer"
        );

    const element =
        document.createElement(
            "div"
        );

    element.className =
        "toast";

    element.textContent =
        message;

    container.appendChild(
        element
    );

    setTimeout(() => {

        element.style.opacity =
            "0";

        setTimeout(() => {

            element.remove();

        }, 250);

    }, 2300);

}


/* =========================================================
   MAIN LOOP
========================================================= */

function gameLoop() {

    updateGameClock();

    updateCustomerLogic();

    updateCustomerSpawner();

    requestAnimationFrame(
        gameLoop
    );

}


/* =========================================================
   AUTO SAVE
========================================================= */

setInterval(
    saveGame,
    5000
);


/* =========================================================
   INITIALIZATION
========================================================= */

function init() {

    loadGame();

    /*
       Nếu save cũ đã bắt đầu,
       không cần hiện Intro nữa.
    */

    if (game.gameStarted) {

        document
            .getElementById(
                "introModal"
            )
            .classList.remove(
                "show"
            );

    } else {

        document
            .getElementById(
                "introModal"
            )
            .classList.add(
                "show"
            );

    }

    renderWarehouse();

    renderUpgrades();

    renderReviews();

    updateUI();

    updateCustomerDOM();

    gameLoop();

}


/* =========================================================
   START ENGINE
========================================================= */

init();
