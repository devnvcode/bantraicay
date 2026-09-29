/* =========================================================
   TIỆM KEM NGỌT NGÀO
   CORE GAME LOGIC
========================================================= */

(() => {

  "use strict";


  /* =======================================================
     SHORTCUTS
  ======================================================= */

  const $ = id => document.getElementById(id);

  const moneyFmt = new Intl.NumberFormat("vi-VN");


  /* =======================================================
     GAME CONFIG
  ======================================================= */

  const CONFIG = {

    startingMoney: 300,

    startingStock: 6,

    baseMaxStock: 12,

    /*
      1 giờ game = 10 giây thực tế.

      09:00 -> 21:00
      12 giờ game = 120 giây thực tế.
    */
    REAL_SECONDS_PER_GAME_HOUR: 10,

    openingAnimationMs: 1500,

    customerEntranceMs: 850,

    customerSpawnDelayMs: 600,

    scoopAnimationMinMs: 320,

    maxReviews: 100

  };


  const GAME_START_MINUTES = 9 * 60;
  const GAME_END_MINUTES = 21 * 60;

  const GAME_TOTAL_MINUTES =
    GAME_END_MINUTES - GAME_START_MINUTES;


  /* =======================================================
     STATE
  ======================================================= */

  const defaultState = () => ({

    money: CONFIG.startingMoney,

    day: 1,

    isOpen: false,

    /*
      Thời gian lưu bằng phút game.
      09:00 lúc mở cửa.
    */
    gameMinutes: GAME_START_MINUTES,

    maxStock: CONFIG.baseMaxStock,

    scoopSpeed: 850,

    freezerLevel: 0,

    staffLevel: 0,

    decorLevel: 0,

    inventory: Object.fromEntries(
      ICE_CREAM_FLAVORS.map(
        flavor => [
          flavor.id,
          CONFIG.startingStock
        ]
      )
    ),

    currentCustomer: null,

    currentOrder: null,

    creation: {

      step: 1,

      base: null,

      scoops: [],

      toppings: []

    },

    daily: {

      revenue: 0,

      supplyCost: 0,

      success: 0,

      fail: 0

    },

    reviews: [],

    usedReviewTexts: []

  });


  let state = loadState();


  /* =======================================================
     RUNTIME TIMERS
  ======================================================= */

  let shopTimer = null;

  let customerTimer = null;

  let scoopTimeout = null;

  let customerSpawnTimeout = null;

  let staffTimeout = null;

  let toastTimer = null;

  let introClosing = false;

  let reviewFilter = "all";


  /* =======================================================
     SAFE HELPERS
  ======================================================= */

  function money(value) {

    return `${moneyFmt.format(
      Math.max(0, Math.round(value))
    )}đ`;

  }


  function rand(array) {

    if (!array || !array.length) {
      return null;
    }

    return array[
      Math.floor(
        Math.random() * array.length
      )
    ];

  }


  function clamp(value, min, max) {

    return Math.min(
      max,
      Math.max(min, value)
    );

  }


  function getFlavor(id) {

    return ICE_CREAM_FLAVORS.find(
      flavor => flavor.id === id
    );

  }


  function getTopping(id) {

    return TOPPINGS.find(
      topping => topping.id === id
    );

  }


  function escapeHtml(value) {

    return String(value)
      .replace(/[&<>'"]/g, character => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        "'": "&#39;",
        "\"": "&quot;"
      }[character]));

  }


  function formatClock(minutes) {

    const safe = clamp(
      Math.floor(minutes),
      GAME_START_MINUTES,
      GAME_END_MINUTES
    );

    const hours = Math.floor(
      safe / 60
    );

    const mins = safe % 60;

    return (
      String(hours).padStart(2, "0") +
      ":" +
      String(mins).padStart(2, "0")
    );

  }


  function minutesToProgress(minutes) {

    return clamp(
      (
        (minutes - GAME_START_MINUTES) /
        GAME_TOTAL_MINUTES
      ) * 360,
      0,
      360
    );

  }


  /* =======================================================
     LOCAL STORAGE
  ======================================================= */

  function saveState() {

    try {

      const copy = JSON.parse(
        JSON.stringify(state)
      );

      copy.isOpen = false;

      localStorage.setItem(
        "sweetIceCreamShop_v2",
        JSON.stringify(copy)
      );

    } catch (error) {

      console.warn(
        "Không thể lưu game:",
        error
      );

    }

  }


  function loadState() {

    try {

      const raw =
        localStorage.getItem(
          "sweetIceCreamShop_v2"
        ) ||
        localStorage.getItem(
          "sweetIceCreamShop_v1"
        );

      if (!raw) {

        const fresh = defaultState();

        seedReviews(fresh);

        return fresh;

      }


      const parsed = JSON.parse(raw);

      const base = defaultState();

      const merged = {

        ...base,

        ...parsed,

        isOpen: false,

        daily: {
          ...base.daily,
          ...(parsed.daily || {})
        },

        creation: {
          ...base.creation,
          ...(parsed.creation || {})
        },

        inventory: {
          ...base.inventory,
          ...(parsed.inventory || {})
        }

      };


      /*
        Migration từ bản cũ:
        bản cũ dùng daySeconds = 120.
      */

      if (
        typeof parsed.gameMinutes !== "number"
      ) {

        merged.gameMinutes =
          GAME_START_MINUTES;

      }


      merged.currentCustomer = null;

      merged.currentOrder = null;

      merged.creation = {
        step: 1,
        base: null,
        scoops: [],
        toppings: []
      };


      merged.isOpen = false;

      merged.reviews =
        Array.isArray(parsed.reviews)
          ? parsed.reviews
          : [];


      if (!merged.reviews.length) {
        seedReviews(merged);
      }


      return merged;

    } catch (error) {

      console.warn(
        "Save cũ không hợp lệ, tạo game mới."
      );

      const fresh = defaultState();

      seedReviews(fresh);

      return fresh;

    }

  }


  function seedReviews(targetState) {

    const amount = 10;

    targetState.reviews = [];

    targetState.usedReviewTexts = [];


    for (let i = 0; i < amount; i++) {

      const text =
        POSITIVE_REVIEWS[i];

      targetState.reviews.push({

        id: `base-${i}-${Date.now()}`,

        name:
          CUSTOMER_NAMES[i],

        avatar:
          AVATARS[i % AVATARS.length],

        stars: 5,

        text,

        time:
          `${i + 1} ngày trước`

      });

      targetState.usedReviewTexts.push(
        text
      );

    }

  }


  /* =======================================================
     TOAST
  ======================================================= */

  function showToast(message) {

    const toast = $("toast");

    if (!toast) {
      return;
    }

    toast.textContent = message;

    toast.classList.add("show");

    clearTimeout(toastTimer);

    toastTimer = setTimeout(() => {

      toast.classList.remove("show");

    }, 2200);

  }


  /* =======================================================
     TOP UI
  ======================================================= */

  function updateTopStats() {

    $("moneyValue").textContent =
      moneyFmt.format(
        Math.round(state.money)
      );

    $("dayValue").textContent =
      state.day;

    $("warehouseMoney").textContent =
      moneyFmt.format(
        Math.round(state.money)
      );

    $("upgradeMoney").textContent =
      moneyFmt.format(
        Math.round(state.money)
      );

    $("maxStockValue").textContent =
      state.maxStock;


    $("openCloseBtn").textContent =
      state.isOpen
        ? "ĐÓNG CỬA"
        : "MỞ CỬA TIỆM";


    $("shopStatusText").textContent =
      state.isOpen

        ? `Tiệm đang mở cửa — giờ hiện tại ${formatClock(state.gameMinutes)}. Phục vụ khách thật nhanh nhé!`

        : "Tiệm đang đóng cửa. Hãy mở cửa để đón khách!";

  }


  /* =======================================================
     DAY CLOCK
  ======================================================= */

  function updateDayClockUI() {

    const time =
      formatClock(
        state.gameMinutes
      );

    $("dayTimer").textContent =
      time;


    const progress =
      minutesToProgress(
        state.gameMinutes
      );


    const ring =
      $("dayClockRing");


    ring.style.setProperty(
      "--clock-progress",
      `${progress}deg`
    );


    const remaining =
      GAME_END_MINUTES -
      state.gameMinutes;


    ring.classList.toggle(
      "warning",
      remaining <= 180 &&
      remaining > 60
    );


    ring.classList.toggle(
      "danger",
      remaining <= 60
    );

  }


  /* =======================================================
     NAV
  ======================================================= */

  function switchTab(tabId) {

    document
      .querySelectorAll(".tab-panel")
      .forEach(panel => {

        panel.classList.toggle(
          "active",
          panel.id === tabId
        );

      });


    document
      .querySelectorAll(".nav-btn")
      .forEach(button => {

        button.classList.toggle(
          "active",
          button.dataset.tab === tabId
        );

      });


    window.scrollTo({
      top: 0,
      behavior: "smooth"
    });

  }


  /* =======================================================
     INTRO MODAL
  ======================================================= */

  function closeIntro() {

    if (introClosing) {
      return;
    }

    const modal =
      $("introModal");

    if (!modal) {
      return;
    }


    introClosing = true;


    /*
      Chống lỗi nút không bấm được:
      - pointer-events đã được kiểm soát bởi CSS
      - button nằm trên modal-card
      - không dùng display:none ngay lập tức
      - fade-out trước
    */

    modal.classList.add("closing");


    setTimeout(() => {

      modal.classList.remove(
        "visible",
        "closing"
      );

      modal.style.pointerEvents =
        "none";

      introClosing = false;

      showToast(
        "Tiệm đã sẵn sàng. Bấm “MỞ CỬA TIỆM” nhé! 🍦"
      );

    }, 240);

  }


  /* =======================================================
     OPEN SHOP
  ======================================================= */

  function openShop() {

    if (state.isOpen) {
      return;
    }


    if (
      $("daySummaryModal")
        .classList
        .contains("visible")
    ) {

      return;

    }


    state.isOpen = true;

    state.gameMinutes =
      GAME_START_MINUTES;


    /*
      Reset thống kê ngày chỉ khi bắt đầu
      một ngày mới thực sự.
    */
    state.daily = {
      revenue: 0,
      supplyCost: 0,
      success: 0,
      fail: 0
    };


    clearCustomerState();


    updateTopStats();

    updateDayClockUI();

    updateOrderUI();


    const overlay =
      $("openingOverlay");


    overlay.classList.add(
      "active"
    );


    requestAnimationFrame(() => {

      requestAnimationFrame(() => {

        overlay.classList.add(
          "play"
        );

      });

    });


    /*
      Cho phép bắt đầu đón khách
      ngay khi rèm mở.
    */
    customerSpawnTimeout =
      setTimeout(() => {

        if (
          state.isOpen &&
          !state.currentCustomer
        ) {

          createCustomer();

        }

      }, CONFIG.openingAnimationMs);


    setTimeout(() => {

      overlay.classList.remove(
        "play"
      );

      setTimeout(() => {

        overlay.classList.remove(
          "active"
        );

      }, 100);

    }, CONFIG.openingAnimationMs);


    startDayClock();

    saveState();

  }


  /* =======================================================
     CLOSE SHOP MANUALLY
  ======================================================= */

  function manualCloseShop() {

    if (!state.isOpen) {
      return;
    }

    finishDay(
      false
    );

  }


  /* =======================================================
     GAME CLOCK
  ======================================================= */

  function startDayClock() {

    clearInterval(shopTimer);


    /*
      10 giây thật = 60 phút game.
      Vì vậy 1 giờ game = 10 giây.
    */

    const gameMinutesPerTick =
      6;


    shopTimer =
      setInterval(() => {

        if (!state.isOpen) {
          return;
        }


        state.gameMinutes +=
          gameMinutesPerTick;


        if (
          state.gameMinutes >=
          GAME_END_MINUTES
        ) {

          state.gameMinutes =
            GAME_END_MINUTES;

          updateDayClockUI();

          finishDay(true);

          return;

        }


        updateDayClockUI();

        updateTopStats();

      }, 1000);

  }


  /* =======================================================
     FINISH DAY
  ======================================================= */

  function finishDay(autoClose = false) {

    if (!state.isOpen) {
      return;
    }


    state.isOpen = false;


    clearInterval(shopTimer);

    shopTimer = null;


    clearInterval(customerTimer);

    customerTimer = null;


    clearTimeout(scoopTimeout);

    scoopTimeout = null;


    clearTimeout(customerSpawnTimeout);

    customerSpawnTimeout = null;


    clearTimeout(staffTimeout);

    staffTimeout = null;


    /*
      Khách đang đứng quầy sẽ rời đi.
      Không tạo review thất bại khi người chơi
      đóng cửa giữa ngày.
    */
    clearCustomerState();


    updateTopStats();

    updateDayClockUI();

    updateOrderUI();


    renderSummary();


    $("daySummaryModal")
      .classList
      .add("visible");


    saveState();


    if (autoClose) {

      showToast(
        "Đã đúng 21:00 — tiệm tự động đóng cửa! 🌙"
      );

    }

  }


  /* =======================================================
     NEW DAY
  ======================================================= */

  function newDay() {

    $("daySummaryModal")
      .classList
      .remove("visible");


    state.day += 1;

    state.gameMinutes =
      GAME_START_MINUTES;

    state.isOpen = false;


    state.daily = {
      revenue: 0,
      supplyCost: 0,
      success: 0,
      fail: 0
    };


    clearCustomerState();


    updateTopStats();

    updateDayClockUI();

    updateOrderUI();

    saveState();


    showToast(
      `Ngày ${state.day} đã sẵn sàng. Mở cửa thôi! 🌷`
    );

  }


  /* =======================================================
     CUSTOMER PATIENCE
  ======================================================= */

  function getCustomerPatience() {

    return (
      30 +
      state.decorLevel * 4
    );

  }


  /* =======================================================
     CREATE CUSTOMER
  ======================================================= */

  function createCustomer() {

    if (
      !state.isOpen ||
      state.currentCustomer
    ) {

      return;

    }


    const firstFlavor =
      rand(ICE_CREAM_FLAVORS);


    /*
      Một số khách gọi 2 viên.
      Giữ tỷ lệ đơn 2 viên thấp
      để game không quá khó.
    */
    const wantsTwoScoops =
      Math.random() < .28;


    const secondFlavor =
      wantsTwoScoops
        ? rand(
            ICE_CREAM_FLAVORS.filter(
              flavor =>
                flavor.id !==
                firstFlavor.id
            )
          )
        : null;


    const flavors =
      secondFlavor
        ? [
            firstFlavor.id,
            secondFlavor.id
          ]
        : [
            firstFlavor.id
          ];


    const toppingCount =
      Math.random() < .58
        ? (Math.random() < .25 ? 2 : 1)
        : 0;


    const shuffledToppings =
      [...TOPPINGS]
        .sort(() => Math.random() - .5);


    const toppings =
      shuffledToppings
        .slice(0, toppingCount)
        .map(topping => topping.id);


    const gender =
      Math.random() > .5
        ? "female"
        : "male";


    const customer = {

      id:
        `${Date.now()}-${Math.random()}`,

      name:
        rand(CUSTOMER_NAMES),

      avatar:
        rand(AVATARS),

      gender,

      hair:
        rand(HAIR_TYPES),

      shirt:
        rand(SHIRT_COLORS),

      skin:
        rand(SKIN_TONES),

      order: {

        scoops: flavors,

        toppings

      },

      patience:
        getCustomerPatience(),

      maxPatience:
        getCustomerPatience(),

      phase:
        "entering"

    };


    state.currentCustomer =
      customer;


    state.currentOrder =
      customer.order;


    resetCreation(false);


    renderCustomer();

    updateOrderUI();


    startCustomerTimer();


    setTimeout(() => {

      if (
        state.currentCustomer &&
        state.currentCustomer.id ===
          customer.id
      ) {

        customer.phase =
          "ready";

        scheduleStaffService();

      }

    }, CONFIG.customerEntranceMs);

  }


  /* =======================================================
     CUSTOMER SVG
  ======================================================= */

  function buildCustomerSvg(
    customer,
    angry = false
  ) {

    const skin =
      angry
        ? "#ef9b95"
        : customer.skin;


    let hair;


    if (
      customer.hair === "cap"
    ) {

      hair = `
        <path
          d="M25 35 Q52 10 79 35 L75 45
             Q52 32 29 46Z"
          fill="#e0b3cf"
        />

        <path
          d="M25 34 Q52 9 79 34"
          fill="none"
          stroke="#9d7591"
          stroke-width="4"
          stroke-linecap="round"
        />
      `;

    }

    else if (
      customer.hair === "long"
    ) {

      hair = `
        <path
          d="M23 48 Q20 15 52 15
             Q84 15 81 48
             L74 79 L63 68 L58 78
             L45 68 L36 80Z"
          fill="#704b3c"
        />
      `;

    }

    else if (
      customer.hair === "curly"
    ) {

      hair = `
        <g fill="#6d4b3b">

          <circle cx="31" cy="31" r="10"/>
          <circle cx="40" cy="21" r="11"/>
          <circle cx="53" cy="18" r="12"/>
          <circle cx="66" cy="23" r="11"/>
          <circle cx="75" cy="34" r="10"/>
          <circle cx="33" cy="43" r="9"/>
          <circle cx="73" cy="44" r="9"/>

        </g>
      `;

    }

    else if (
      customer.hair === "bob"
    ) {

      hair = `
        <path
          d="M23 48 Q23 17 52 14
             Q80 18 82 48
             L75 63
             Q67 29 52 30
             Q37 30 30 63Z"
          fill="#704b3c"
        />
      `;

    }

    else {

      hair = `
        <path
          d="M27 44 Q28 16 52 14
             Q77 16 78 44
             L70 35
             Q60 27 52 27
             Q43 27 34 35Z"
          fill="#684639"
        />
      `;

    }


    const shirt = `
      <path
        d="M30 88
           Q52 77 74 88
           L84 132
           L20 132Z"
        fill="${customer.shirt}"
      />
    `;


    const eyes = `
      <path
        d="M39 51 q3 -3 6 0"
        stroke="#4b403d"
        stroke-width="3"
        fill="none"
        stroke-linecap="round"
      />

      <path
        d="M60 51 q3 -3 6 0"
        stroke="#4b403d"
        stroke-width="3"
        fill="none"
        stroke-linecap="round"
      />
    `;


    const cheeks = `
      <circle
        cx="37"
        cy="60"
        r="5"
        fill="#ee9b9b"
        opacity=".65"
      />

      <circle
        cx="69"
        cy="60"
        r="5"
        fill="#ee9b9b"
        opacity=".65"
      />
    `;


    const mouth =
      angry

        ? `
          <path
            d="M44 70 Q52 64 60 70"
            stroke="#704a42"
            stroke-width="2.5"
            fill="none"
            stroke-linecap="round"
          />
        `

        : `
          <path
            d="M45 66 Q52 71 59 66"
            stroke="#704a42"
            stroke-width="2.4"
            fill="none"
            stroke-linecap="round"
          />
        `;


    return `

      <svg
        class="customer-svg"
        viewBox="0 0 104 140"
        xmlns="http://www.w3.org/2000/svg"
        aria-hidden="true"
      >

        ${shirt}

        <rect
          x="46"
          y="72"
          width="12"
          height="18"
          rx="5"
          fill="${skin}"
        />

        <ellipse
          cx="52"
          cy="50"
          rx="27"
          ry="29"
          fill="${skin}"
        />

        ${hair}

        ${eyes}

        ${cheeks}

        ${mouth}

        <path
          d="M22 94
             Q52 79 82 94"
          stroke="rgba(75,64,61,.12)"
          stroke-width="4"
          fill="none"
        />

        <rect
          x="33"
          y="126"
          width="18"
          height="11"
          rx="5"
          fill="#706f7a"
        />

        <rect
          x="55"
          y="126"
          width="18"
          height="11"
          rx="5"
          fill="#706f7a"
        />

      </svg>
    `;

  }


  /* =======================================================
     CUSTOMER ORDER TEXT
  ======================================================= */

  function buildOrderText(customer) {

    const order =
      customer.order.scoops
        .map(
          id =>
            `1 viên Kem ${getFlavor(id).name}`
        )
        .join(" + ");


    const toppingText =
      customer.order.toppings.length

        ? ` + ${customer.order.toppings
            .map(
              id =>
                getTopping(id).name
            )
            .join(", ")}`

        : "";


    return order + toppingText;

  }


  function requestText(customer) {

    const order =
      buildOrderText(customer);


    const list =
      CUSTOMER_REQUEST_TEMPLATES[
        customer.gender
      ];


    const template =
      list[
        (
          customer.name
            .charCodeAt(0) +
          state.day
        ) %
        list.length
      ];


    return template.replace(
      "{order}",
      order
    );

  }


  /* =======================================================
     RENDER CUSTOMER
  ======================================================= */

  function renderCustomer(
    angry = false
  ) {

    const stage =
      $("customerStage");


    if (
      !state.currentCustomer
    ) {

      stage.innerHTML = "";

      return;

    }


    const customer =
      state.currentCustomer;


    const pct =
      clamp(
        (
          customer.patience /
          customer.maxPatience
        ) * 100,
        0,
        100
      );


    const ringClass =
      pct > 60
        ? ""
        : pct > 30
          ? "warn"
          : "danger";


    const bubbleText =
      angry
        ? rand(
            NEGATIVE_REVIEWS.slice(0, 8)
          )
        : requestText(customer);


    stage.innerHTML = `

      <div
        class="customer-card ${angry ? "leaving" : ""}"
      >

        <div class="patience-wrap">

          <div
            class="patience-ring ${ringClass}"
            style="--p:${pct}"
          >

            <span>
              ${Math.ceil(
                customer.patience
              )}
            </span>

          </div>

        </div>


        <div class="customer-bubble">
          ${escapeHtml(
            bubbleText
          )}
        </div>


        ${buildCustomerSvg(
          customer,
          angry
        )}


        <div class="customer-name-tag">
          ${escapeHtml(
            customer.name
          )}
        </div>

      </div>
    `;


    const card =
      stage.querySelector(
        ".customer-card"
      );


    if (
      card &&
      !angry
    ) {

      card.addEventListener(
        "click",
        deliverCustomer
      );

    }

  }


  /* =======================================================
     CUSTOMER TIMER
  ======================================================= */

  function startCustomerTimer() {

    clearInterval(
      customerTimer
    );


    customerTimer =
      setInterval(() => {

        if (
          !state.isOpen ||
          !state.currentCustomer
        ) {

          return;

        }


        const customer =
          state.currentCustomer;


        customer.patience =
          Math.max(
            0,
            customer.patience - 1
          );


        updateCustomerVisual();

        updateOrderUI();


        if (
          customer.patience <= 0
        ) {

          customerTimeout();

        }

      }, 1000);

  }


  function updateCustomerVisual() {

    const customer =
      state.currentCustomer;


    if (!customer) {
      return;
    }


    const ring =
      document.querySelector(
        ".patience-ring"
      );


    const bubble =
      document.querySelector(
        ".customer-bubble"
      );


    if (!ring || !bubble) {
      return;
    }


    const pct =
      clamp(
        (
          customer.patience /
          customer.maxPatience
        ) * 100,
        0,
        100
      );


    ring.style.setProperty(
      "--p",
      pct
    );


    ring.classList.toggle(
      "warn",
      pct <= 60 && pct > 30
    );


    ring.classList.toggle(
      "danger",
      pct <= 30
    );


    const number =
      ring.querySelector("span");


    if (number) {

      number.textContent =
        Math.ceil(
          customer.patience
        );

    }


    bubble.textContent =
      requestText(customer);

  }


  /* =======================================================
     CUSTOMER TIMEOUT
  ======================================================= */

  function customerTimeout() {

    if (
      !state.currentCustomer
    ) {

      return;

    }


    clearInterval(
      customerTimer
    );


    customerTimer = null;


    const customer =
      state.currentCustomer;


    state.daily.fail += 1;


    pushReview(
      1
    );


    renderCustomer(
      true
    );


    showToast(
      `${customer.name}: “${rand(
        NEGATIVE_REVIEWS
      )}” 😡`
    );


    /*
      Không tạo khách mới ngay.
      Đợi khách rời màn hình.
    */

    state.currentCustomer = null;

    state.currentOrder = null;

    resetCreation(false);


    saveState();


    setTimeout(() => {

      $("customerStage")
        .innerHTML = "";


      if (
        state.isOpen &&
        !state.currentCustomer
      ) {

        customerSpawnTimeout =
          setTimeout(
            createCustomer,
            CONFIG.customerSpawnDelayMs
          );

      }

    }, 750);

  }


  /* =======================================================
     CREATION RESET
  ======================================================= */

  function resetCreation(
    refund = true
  ) {

    if (
      refund &&
      state.creation.scoops.length
    ) {

      state.creation.scoops
        .forEach(id => {

          state.inventory[id] =
            Math.min(
              state.maxStock,
              (
                state.inventory[id] ||
                0
              ) + 1
            );

        });

    }


    state.creation = {

      step: 1,

      base: null,

      scoops: [],

      toppings: []

    };


    clearTimeout(
      scoopTimeout
    );


    scoopTimeout = null;


    renderCreation();

    renderTubs();

    renderWarehouse();

  }


  /* =======================================================
     CREATION STEP UI
  ======================================================= */

  function renderCreationSteps() {

    const step =
      state.creation.step;


    $("creationStepLabel")
      .textContent =
      `BƯỚC ${step}/3`;


    [1, 2, 3].forEach(number => {

      const dot =
        $(`stepDot${number}`);


      if (!dot) {
        return;
      }


      dot.classList.toggle(
        "active",
        number === step
      );


      dot.classList.toggle(
        "done",
        number < step
      );

    });

  }


  /* =======================================================
     RENDER DESSERT
  ======================================================= */

  function renderDessert() {

    const display =
      $("iceCreamDisplay");


    const creation =
      state.creation;


    if (!creation.base) {

      display.innerHTML = `

        <div class="empty-dessert">

          <div class="empty-dessert-icon">
            🍨
          </div>

          <strong>
            Chọn đế kem để bắt đầu
          </strong>

          <span>
            Làm từng bước để hoàn thành đơn
          </span>

        </div>
      `;

      return;

    }


    const baseClass =
      creation.base === "cone"
        ? "dessert-cone"
        : "dessert-cup";


    const scoopsHtml =
      creation.scoops
        .map(
          flavorId => {

            const flavor =
              getFlavor(
                flavorId
              );

            return `

              <div
                class="scoop"
                style="--scoop-color:${flavor.color}"
                title="${escapeHtml(
                  flavor.name
                )}"
              ></div>

            `;

          }
        )
        .join("");


    const toppingHtml =
      creation.toppings
        .map(
          toppingId => {

            const topping =
              getTopping(
                toppingId
              );

            return `

              <span
                class="dessert-topping"
                style="
                  left:${20 + Math.random() * 75}%;
                  top:${Math.random() * 28}px;
                "
              >
                ${topping.emoji}
              </span>

            `;

          }
        )
        .join("");


    display.innerHTML = `

      <div class="dessert">

        <div class="scoop-stack">
          ${scoopsHtml}
        </div>


        <div class="dessert-toppings">
          ${toppingHtml}
        </div>


        <div class="dessert-base">

          <div class="${baseClass}">
          </div>

        </div>

      </div>
    `;

  }


  /* =======================================================
     RENDER STEP 1
  ======================================================= */

  function renderBaseChoices() {

    document
      .querySelectorAll(
        ".base-choice"
      )
      .forEach(button => {

        button.classList.toggle(
          "selected",
          button.dataset.base ===
            state.creation.base
        );

      });

  }


  /* =======================================================
     RENDER STEP 2
  ======================================================= */

  function renderWorkspaceFlavors() {

    const container =
      $("workspaceFlavorButtons");


    if (!state.currentCustomer) {

      container.innerHTML = `
        <div class="workspace-hint">
          Chưa có khách.
        </div>
      `;

      return;

    }


    const requested =
      state.currentCustomer
        .order
        .scoops;


    container.innerHTML =
      ICE_CREAM_FLAVORS
        .map(flavor => {

          const stock =
            state.inventory[
              flavor.id
            ] || 0;


          const requestedCount =
            requested.filter(
              id =>
                id === flavor.id
            ).length;


          const currentCount =
            state.creation.scoops
              .filter(
                id =>
                  id === flavor.id
              )
              .length;


          const canSelect =
            requestedCount >
              currentCount &&
            stock > 0;


          return `

            <button
              type="button"
              class="
                workspace-flavor
                ${!canSelect ? "empty" : ""}
                ${currentCount > 0 ? "selected" : ""}
              "
              data-workspace-flavor="${flavor.id}"
              ${canSelect ? "" : "disabled"}
            >

              <span>
                ${flavor.emoji}
              </span>

              <strong>
                ${escapeHtml(
                  flavor.name
                )}
              </strong>

              <small>
                ${stock}/${state.maxStock}
              </small>

            </button>
          `;

        })
        .join("");


    container
      .querySelectorAll(
        "[data-workspace-flavor]"
      )
      .forEach(button => {

        button.addEventListener(
          "click",
          () => {

            scoopFlavor(
              button.dataset
                .workspaceFlavor
            );

          }
        );

      });

  }


  /* =======================================================
     RENDER STEP 3
  ======================================================= */

  function renderWorkspaceToppings() {

    const container =
      $("workspaceToppingButtons");


    if (!state.currentCustomer) {

      container.innerHTML = "";

      return;

    }


    const requested =
      state.currentCustomer
        .order
        .toppings;


    container.innerHTML =
      TOPPINGS
        .map(topping => {

          const wanted =
            requested.includes(
              topping.id
            );


          const selected =
            state.creation.toppings
              .includes(
                topping.id
              );


          return `

            <button
              type="button"
              class="
                workspace-topping
                ${selected ? "selected" : ""}
                ${!wanted ? "empty" : ""}
              "
              data-workspace-topping="${topping.id}"
              ${wanted ? "" : "disabled"}
            >

              <span>
                ${topping.emoji}
              </span>

              <strong>
                ${escapeHtml(
                  topping.name
                )}
              </strong>

            </button>
          `;

        })
        .join("");


    container
      .querySelectorAll(
        "[data-workspace-topping]"
      )
      .forEach(button => {

        button.addEventListener(
          "click",
          () => {

            selectTopping(
              button.dataset
                .workspaceTopping
            );

          }
        );

      });

  }


  /* =======================================================
     RENDER CREATION
  ======================================================= */

  function renderCreation() {

    renderCreationSteps();

    renderBaseChoices();

    renderDessert();

    renderWorkspaceFlavors();

    renderWorkspaceToppings();


    $("baseChoices")
      .classList.toggle(
        "hidden",
        state.creation.step !== 1
      );


    $("scoopWorkspace")
      .classList.toggle(
        "hidden",
        state.creation.step !== 2
      );


    $("toppingWorkspace")
      .classList.toggle(
        "hidden",
        state.creation.step !== 3
      );


    const complete =
      isCreationComplete();


    $("serveBtn").disabled =
      !complete ||
      !state.currentCustomer ||
      !state.isOpen;


    $("resetScoopBtn").disabled =
      !(
        state.creation.base ||
        state.creation.scoops.length ||
        state.creation.toppings.length
      );


    $("backStepBtn").disabled =
      state.creation.step === 1;

  }


  /* =======================================================
     SELECT BASE
  ======================================================= */

  function selectBase(base) {

    if (
      !state.currentCustomer ||
      !state.isOpen
    ) {

      showToast(
        "Hãy mở cửa và đón khách trước nhé!"
      );

      return;

    }


    state.creation.base =
      base;


    state.creation.step =
      2;


    renderCreation();

    showToast(
      base === "cone"
        ? "Đã chọn Vỏ Ốc Quế 🍦"
        : "Đã chọn Ly Kem 🍨"
    );

  }


  /* =======================================================
     SCOOP
  ======================================================= */

  function scoopFlavor(
    flavorId
  ) {

    if (
      !state.isOpen ||
      !state.currentCustomer
    ) {

      showToast(
        "Mở cửa và đón khách trước nhé!"
      );

      return;

    }


    if (!state.creation.base) {

      showToast(
        "Bước 1: hãy chọn đế kem trước."
      );

      state.creation.step = 1;

      renderCreation();

      return;

    }


    if (
      state.creation.step < 2
    ) {

      state.creation.step = 2;

    }


    const order =
      state.currentCustomer
        .order
        .scoops;


    const wantedCount =
      order.filter(
        id =>
          id === flavorId
      ).length;


    const currentCount =
      state.creation.scoops
        .filter(
          id =>
            id === flavorId
        ).length;


    if (
      currentCount >=
      wantedCount
    ) {

      showToast(
        `Khách không gọi thêm vị ${getFlavor(flavorId).name}.`
      );

      return;

    }


    const stock =
      state.inventory[
        flavorId
      ] || 0;


    if (stock <= 0) {

      showToast(
        "Hũ này hết kem rồi! Vào Kho Hàng nhập thêm nhé."
      );

      return;

    }


    /*
      Không trừ kho ngay lúc click.
      Chỉ trừ khi animation múc hoàn thành.
      Nếu khách timeout trong lúc animation,
      inventory vẫn an toàn.
    */

    const targetCustomerId =
      state.currentCustomer.id;


    const tub =
      document.querySelector(
        `.tub[data-flavor="${flavorId}"]`
      );


    tub?.classList.add(
      "mini-shake"
    );


    setTimeout(() => {

      tub?.classList.remove(
        "mini-shake"
      );

    }, 320);


    clearTimeout(
      scoopTimeout
    );


    scoopTimeout =
      setTimeout(() => {

        /*
          Customer có thể đã timeout.
          Kiểm tra ID trước khi thay đổi state.
        */

        if (
          !state.currentCustomer ||
          state.currentCustomer.id !==
            targetCustomerId
        ) {

          return;

        }


        if (
          !state.isOpen
        ) {

          return;

        }


        if (
          (state.inventory[
            flavorId
          ] || 0) <= 0
        ) {

          return;

        }


        state.inventory[
          flavorId
        ] -= 1;


        state.creation.scoops
          .push(
            flavorId
          );


        renderTubs();

        renderWarehouse();

        renderCreation();


        showToast(
          `Đã múc ${getFlavor(flavorId).name}! 🥄`
        );


        /*
          Nếu đủ số viên -> sang topping.
        */

        if (
          state.creation.scoops
            .length ===
          order.length
        ) {

          state.creation.step = 3;

          renderCreation();

        }


        saveState();

      }, Math.max(
        CONFIG.scoopAnimationMinMs,
        state.scoopSpeed
      ));

  }


  /* =======================================================
     TOPPING
  ======================================================= */

  function selectTopping(
    toppingId
  ) {

    if (
      !state.currentCustomer ||
      !state.isOpen
    ) {

      return;

    }


    const wanted =
      state.currentCustomer
        .order
        .toppings
        .includes(
          toppingId
        );


    if (!wanted) {

      showToast(
        "Khách không gọi topping này."
      );

      return;

    }


    if (
      state.creation.toppings
        .includes(
          toppingId
        )
    ) {

      showToast(
        "Topping này đã được rắc rồi."
      );

      return;

    }


    state.creation.toppings
      .push(
        toppingId
      );


    renderCreation();


    showToast(
      `Đã thêm ${getTopping(toppingId).name}! ✨`
    );


    saveState();

  }


  /* =======================================================
     CREATION COMPLETE
  ======================================================= */

  function isCreationComplete() {

    const customer =
      state.currentCustomer;


    if (!customer) {
      return false;
    }


    if (!state.creation.base) {
      return false;
    }


    const requiredScoops =
      customer.order.scoops;


    if (
      state.creation
