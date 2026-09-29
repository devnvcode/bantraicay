// ====== DỮ LIỆU GAME (DATABASE) ======
const FLAVORS = [
    { id: 'choco', name: 'Sô-cô-la', color: '#6F4E37', price: 10 },
    { id: 'straw', name: 'Dâu Tây', color: '#FFB7C5', price: 10 },
    { id: 'vani', name: 'Vani', color: '#F3E5AB', price: 10 },
    { id: 'matcha', name: 'Matcha', color: '#C8E6C9', price: 10 },
    { id: 'avocado', name: 'Kem Bơ', color: '#A8E4A0', price: 15 },
    { id: 'durian', name: 'Sầu Riêng', color: '#FFD700', price: 15 },
    { id: 'taro', name: 'Khoai Môn', color: '#D8BFD8', price: 12 },
    { id: 'oreo', name: 'Oreo', color: '#424242', price: 12 },
    { id: 'yogurt', name: 'Sữa Chua', color: '#FFFFFF', price: 10 },
    { id: 'rainbow', name: '7 Màu', color: 'linear-gradient(45deg, red, yellow, green, blue)', price: 20 }
];

const CUSTOMER_NAMES = ["Hoàng Nam", "Thu Trang", "Bảo Anh", "Ngọc Linh", "Minh Đức", "Mai Phương", "Tuấn Kiệt", "Khánh An", "Phương Ly", "Hải Đăng"];
const REVIEWS_5_STAR = [
    "Kem ở đây ngon xỉu, chủ shop múc nhanh nhiệt tình!",
    "10/10 điểm, kem béo ngậy ăn là ghiền!",
    "Tiệm xinh xắn, giao kem nhanh thoăn thoắt ❤️",
    "Tuyệt vời! Sẽ rủ cả công ty ra ăn.",
    "Đỉnh chóp, kem ngon nhất cái phố này luôn."
];
const REVIEWS_1_STAR = [
    "Bắt khách chờ héo cả người, dẹp tiệm đi!",
    "Chậm như rùa, cạch mặt shop này luôn!",
    "Đợi lâu muốn xỉu mà không thấy kem đâu!",
    "Phục vụ quá tệ, 1 sao không nói nhiều.",
    "Bán buôn kiểu gì gọi món xong đứng ngó?"
];
const ANGRY_QUOTES = ["Múc kem thôi mà lâu thế!", "Chờ héo cả người, đi về!", "Thôi dẹp dẹp, cạch mặt!"];

// ====== BIẾN TRẠNG THÁI GAME ======
let money = 100;
let timeLeft = 60; // 1 ngày = 60 giây
let isDayRunning = false;
let timerInterval;

let inventory = {}; // Số lượng kem mỗi vị
let currentCone = []; // Viên kem đang múc
let currentCustomer = null; // Khách hiện tại

let stats = { revenue: 0, cost: 0, success: 0, fail: 0 };
let allReviews = [];

// Khởi tạo kho hàng (mỗi vị 5 viên)
FLAVORS.forEach(f => inventory[f.id] = 5);

// ====== HÀM KHỞI TẠO & RENDER ======
function initGame() {
    renderTubs();
    renderInventory();
    updateHeader();
    
    // Gắn sự kiện Bottom Nav
    document.querySelectorAll('.nav-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
            e.currentTarget.classList.add('active');
            document.getElementById(e.currentTarget.dataset.target).classList.add('active');
        });
    });

    // Nút Bắt đầu
    document.getElementById('btn-start-intro').addEventListener('click', () => {
        document.getElementById('intro-modal').classList.add('hidden');
        startDayTransition();
    });

    // Nút Ngày tiếp theo
    document.getElementById('btn-next-day').addEventListener('click', () => {
        document.getElementById('end-day-modal').classList.add('hidden');
        startDayTransition();
    });

    // Các nút quầy
    document.getElementById('btn-trash').addEventListener('click', () => { currentCone = []; renderCone(); });
    document.getElementById('btn-serve').addEventListener('click', serveIceCream);
}

function startDayTransition() {
    const overlay = document.getElementById('transition-screen');
    const curtain = overlay.querySelector('.curtain');
    overlay.classList.remove('hidden');
    curtain.classList.remove('slide-up');
    
    // Setup lại ngày mới
    timeLeft = 60;
    stats = { revenue: 0, cost: 0, success: 0, fail: 0 };
    updateHeader();
    currentCone = []; renderCone();
    
    // Bắt đầu mở màn
    setTimeout(() => {
        curtain.classList.add('slide-up');
        setTimeout(() => {
            overlay.classList.add('hidden');
            startGameLoop();
        }, 1200);
    }, 500);
}

// ====== LOGIC RENDER UI ======
function renderTubs() {
    const grid = document.getElementById('tubs-grid');
    grid.innerHTML = '';
    FLAVORS.forEach(f => {
        const qty = inventory[f.id];
        const tub = document.createElement('div');
        tub.className = `tub ${qty === 0 ? 'empty' : ''}`;
        tub.style.background = f.color;
        tub.innerHTML = `<span>${f.name}</span><span>(${qty})</span>`;
        
        tub.addEventListener('click', () => scoopIceCream(f));
        grid.appendChild(tub);
    });
}

function renderInventory() {
    const list = document.getElementById('inventory-list');
    list.innerHTML = '';
    FLAVORS.forEach(f => {
        const item = document.createElement('div');
        item.className = 'inv-item';
        item.innerHTML = `
            <div class="inv-color-box" style="background: ${f.color}"></div>
            <b>${f.name}</b>
            <button class="btn-buy" onclick="buyStock('${f.id}', 10)">Nhập 5 viên (10$)</button>
        `;
        list.appendChild(item);
    });
}

function renderCone() {
    const container = document.getElementById('scoop-container');
    container.innerHTML = '';
    currentCone.forEach(f => {
        const scoop = document.createElement('div');
        scoop.className = 'ice-scoop';
        scoop.style.background = f.color;
        container.appendChild(scoop);
    });
}

function updateHeader() {
    document.getElementById('money').innerText = money;
    document.getElementById('time-left').innerText = timeLeft;
    
    // Tính trung bình sao
    if(allReviews.length === 0) {
        document.getElementById('avg-rating').innerText = "5.0";
    } else {
        const total = allReviews.reduce((sum, r) => sum + r.stars, 0);
        document.getElementById('avg-rating').innerText = (total / allReviews.length).toFixed(1);
    }
}

// ====== LOGIC MÚC/MUA KEM ======
function scoopIceCream(flavor) {
    if (inventory[flavor.id] > 0 && currentCone.length < 3) {
        inventory[flavor.id]--;
        currentCone.push(flavor);
        renderTubs();
        renderCone();
    }
}

window.buyStock = function(flavorId, cost) {
    if (money >= cost) {
        money -= cost;
        inventory[flavorId] += 5;
        stats.cost += cost;
        renderTubs();
        renderInventory();
        updateHeader();
    } else {
        alert("Không đủ tiền nhập hàng!");
    }
};

// ====== LOGIC KHÁCH HÀNG ======
function startGameLoop() {
    isDayRunning = true;
    timerInterval = setInterval(() => {
        timeLeft--;
        updateHeader();
        if(timeLeft <= 0) {
            endDay();
        }
    }, 1000);
    spawnCustomer();
}

function endDay() {
    isDayRunning = false;
    clearInterval(timerInterval);
    document.getElementById('customer-container').classList.add('hidden');
    
    // Hiển thị bảng tổng kết
    document.getElementById('stat-revenue').innerText = `${stats.revenue}$`;
    document.getElementById('stat-cost').innerText = `${stats.cost}$`;
    document.getElementById('stat-profit').innerText = `${stats.revenue - stats.cost}$`;
    document.getElementById('stat-success').innerText = stats.success;
    document.getElementById('stat-fail').innerText = stats.fail;
    
    document.getElementById('end-day-modal').classList.remove('hidden');
}

// Hàm sinh SVG Khách hàng động
function generateCustomerSVG() {
    const hairs = ['#4A2311', '#F5D061', '#111111', '#8C3B1A'];
    const shirts = ['#FF6B6B', '#4ECDC4', '#FFE66D', '#1A535C'];
    const skin = '#FFE0BD';
    const hColor = hairs[Math.floor(Math.random()*hairs.length)];
    const sColor = shirts[Math.floor(Math.random()*shirts.length)];
    
    return `
    <svg viewBox="0 0 100 130" width="100%" height="100%" class="bounce" id="cust-svg">
        <!-- Thân -->
        <rect x="25" y="70" width="50" height="60" rx="10" fill="${sColor}" />
        <!-- Cổ -->
        <rect x="42" y="55" width="16" height="20" fill="${skin}" />
        <!-- Đầu -->
        <circle cx="50" cy="35" r="25" fill="${skin}" />
        <!-- Tóc (Cơ bản) -->
        <path d="M 25 35 Q 50 -5 75 35 Q 75 20 50 10 Q 25 20 25 35" fill="${hColor}" />
        <!-- Mắt -->
        <circle cx="40" cy="35" r="3" fill="#000" class="eye" />
        <circle cx="60" cy="35" r="3" fill="#000" class="eye" />
        <!-- Má hồng -->
        <circle cx="33" cy="42" r="4" fill="#FFB6C1" opacity="0.6"/>
        <circle cx="67" cy="42" r="4" fill="#FFB6C1" opacity="0.6"/>
        <!-- Miệng cười -->
        <path d="M 42 45 Q 50 52 58 45" stroke="#000" stroke-width="2" fill="none" id="cust-mouth" />
    </svg>`;
}

function spawnCustomer() {
    if(!isDayRunning) return;
    const container = document.getElementById('customer-container');
    const svgWrap = document.getElementById('customer-svg-wrapper');
    const chatBubble = document.getElementById('customer-chat');
    const patienceRing = document.getElementById('patience-progress');
    
    // Reset Trạng Thái
    chatBubble.classList.remove('angry');
    patienceRing.style.stroke = "var(--pastel-mint)";
    patienceRing.style.strokeDashoffset = "0";
    
    // Sinh Khách
    const flavor = FLAVORS[Math.floor(Math.random() * FLAVORS.length)];
    const name = CUSTOMER_NAMES[Math.floor(Math.random() * CUSTOMER_NAMES.length)];
    
    currentCustomer = {
        name: name,
        order: flavor,
        patience: 100,
        maxPatience: 100
    };
    
    chatBubble.innerText = `Cho ${name} 1 kem ${flavor.name} nhé!`;
    svgWrap.innerHTML = generateCustomerSVG();
    container.classList.remove('hidden');

    // Chạy đồng hồ
    if(currentCustomer.timer) clearInterval(currentCustomer.timer);
    
    currentCustomer.timer = setInterval(() => {
        if(!isDayRunning) return clearInterval(currentCustomer.timer);
        
        currentCustomer.patience -= 1.5; // Tốc độ giảm kiên nhẫn
        
        // Update Ring (100.53 là chu vi)
        const offset = 100.53 - (currentCustomer.patience / 100) * 100.53;
        patienceRing.style.strokeDashoffset = offset;
        
        // Đổi màu vàng/đỏ
        if(currentCustomer.patience < 50) patienceRing.style.stroke = "#FFD700";
        if(currentCustomer.patience < 25) patienceRing.style.stroke = "#F44336";
        
        if (currentCustomer.patience <= 0) {
            clearInterval(currentCustomer.timer);
            customerAngry();
        }
    }, 100);
}

function customerAngry() {
    const chat = document.getElementById('customer-chat');
    const mouth = document.getElementById('cust-mouth');
    
    chat.classList.add('angry');
    chat.innerText = ANGRY_QUOTES[Math.floor(Math.random() * ANGRY_QUOTES.length)];
    if(mouth) mouth.setAttribute('d', 'M 42 50 Q 50 45 58 50'); // Miệng mếu
    
    stats.fail++;
    addReview(currentCustomer.name, 1);
    
    setTimeout(() => {
        document.getElementById('customer-container').classList.add('hidden');
        setTimeout(spawnCustomer, 1000);
    }, 1500);
}

function serveIceCream() {
    if (!currentCustomer || currentCone.length === 0) return;
    
    // Kiểm tra đúng đơn chưa (Ở đây game cho phép khách đặt 1 vị, player múc đúng 1 vị)
    // Nếu muốn phức tạp hơn có thể duyệt mảng, ở đây kiểm tra viên cuối hoặc viên đầu
    const isCorrect = currentCone.length === 1 && currentCone[0].id === currentCustomer.order.id;
    
    clearInterval(currentCustomer.timer);
    
    if (isCorrect) {
        money += currentCustomer.order.price;
        stats.revenue += currentCustomer.order.price;
        stats.success++;
        addReview(currentCustomer.name, 5);
        document.getElementById('customer-chat').innerText = "Ngon quá! Cảm ơn shop ❤️";
    } else {
        document.getElementById('customer-chat').classList.add('angry');
        document.getElementById('customer-chat').innerText = "Đưa lộn kem rồi shop ơi! 😡";
        stats.fail++;
        addReview(currentCustomer.name, 1);
    }
    
    updateHeader();
    currentCone = [];
    renderCone();
    
    setTimeout(() => {
        document.getElementById('customer-container').classList.add('hidden');
        setTimeout(spawnCustomer, 1000);
    }, 1000);
}

// ====== HỆ THỐNG ĐÁNH GIÁ (REVIEWS) ======
function addReview(name, stars) {
    const content = stars === 5 
        ? REVIEWS_5_STAR[Math.floor(Math.random() * REVIEWS_5_STAR.length)]
        : REVIEWS_1_STAR[Math.floor(Math.random() * REVIEWS_1_STAR.length)];
        
    allReviews.unshift({ name, stars, content }); // Thêm vào đầu
    
    const feed = document.getElementById('reviews-feed');
    const card = document.createElement('div');
    card.className = 'review-card';
    card.innerHTML = `
        <div class="review-head">
            <span>👤 ${name}</span>
            <span class="${stars === 5 ? 'star-5' : 'star-1'}">${'★'.repeat(stars)}${'☆'.repeat(5-stars)}</span>
        </div>
        <p>"${content}"</p>
    `;
    feed.prepend(card);
    updateHeader();
}

// Khởi chạy game
initGame();
