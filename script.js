(() => {

  "use strict";


  /* =========================================================
     HELPERS
  ========================================================= */

  const $ = (id) =>
    document.getElementById(id);

  const moneyFormatter =
    new Intl.NumberFormat("vi-VN");

  const money = (value) =>
    moneyFormatter.format(
      Math.round(value)
    ) + "đ";

  const random = (array) =>
    array[
      Math.floor(
        Math.random() * array.length
      )
    ];


  /* =========================================================
     FLAVORS
  ========================================================= */

  const FLAVORS = [

    {
      id: "chocolate",
      name: "Sô-cô-la",
      emoji: "🍫",
      color: "#79513e",
      price: 24,
      wholesale: 78
    },

    {
      id: "strawberry",
      name: "Dâu Tây",
      emoji: "🍓",
      color: "#ed9cad",
      price: 23,
      wholesale: 76
    },

    {
      id: "vanilla",
      name: "Vani",
      emoji: "🍦",
      color: "#f2e3ad",
      price: 21,
      wholesale: 72
    },

    {
      id: "matcha",
      name: "Matcha",
      emoji: "🍵",
      color: "#a8cf99",
      price: 25,
      wholesale: 82
    },

    {
      id: "durian",
      name: "Sầu Riêng",
      emoji: "🥭",
      color: "#eed878",
      price: 27,
      wholesale: 88
    },

    {
      id: "avocado",
      name: "Bơ",
      emoji: "🥑",
      color: "#b9ce79",
      price: 24,
      wholesale: 80
    },

    {
      id: "oreo",
      name: "Oreo",
      emoji: "🍪",
      color: "#85858c",
      price: 26,
      wholesale: 84
    },

    {
      id: "taro",
      name: "Khoai Môn",
      emoji: "🍠",
      color: "#c6a8dd",
      price: 24,
      wholesale: 80
    },

    {
      id: "yogurt",
      name: "Sữa Chua",
      emoji: "🥛",
      color: "#dce7ef",
      price: 22,
      wholesale: 74
    },

    {
      id: "rainbow",
      name: "Rainbow 7 Màu",
      emoji: "🌈",
      color: "#efbcd0",
      price: 29,
      wholesale: 94
    }

  ];


  /* =========================================================
     CUSTOMERS
  ========================================================= */

  const CUSTOMER_NAMES = [

    "Hoàng Minh",
    "Thảo Nguyên",
    "Bảo Châu",
    "Khánh Linh",
    "Đức Minh",
    "Mai Anh",
    "Tuấn Anh",
    "Ngọc Mai",
    "Gia Bảo",
    "Quang Minh",
    "Hà Linh",
    "Phúc An",
    "Thanh Hà",
    "Mỹ Linh",
    "Anh Tú",
    "Yến Trang",
    "Minh Quân",
    "Linh Chi",
    "Tú Anh",
    "Hải Nam",
    "Phương Anh",
    "Nhật Hạ",
    "Đăng Minh",
    "Thùy Dương",
    "Thiên An",
    "Khôi Vũ",
    "Hồng Nhung",
    "Duy Khánh",
    "Ngân Hà",
    "Trúc Linh",
    "Kiến Văn",
    "Mộc Lan",
    "Vân Anh",
    "Tường Vy",
    "Hoài Nam",
    "Nhã Uyên",
    "Trung Kiên",
    "Diệu Linh",
    "Gia Minh",
    "Thái An",
    "Khánh Vy",
    "Bảo Ngọc"
  ];

  const AVATARS = [
    "🧑🏻",
    "👩🏻",
    "🧑🏼",
    "👩🏼",
    "👨🏻",
    "👩🏽",
    "🧑🏽",
    "👨🏽",
    "👧🏻",
    "👦🏻"
  ];


  /* =========================================================
     REVIEWS
  ========================================================= */

  const FIVE_STAR_REVIEWS = [

    "Múc kem nhanh và rất khéo, viên nào cũng tròn xinh!",
    "Kem matcha thơm dịu, ăn xong vẫn muốn gọi thêm.",
    "Quầy xe kem nhìn yêu quá, phục vụ cũng nhẹ nhàng.",
    "Dâu Tây mềm mịn, vị vừa ngọt vừa thơm.",
    "Ốc quế giòn, kem mát lạnh, trải nghiệm rất dễ chịu.",
    "Sô-cô-la đậm vị nhưng không bị ngọt gắt.",
    "Vani thơm nhẹ, hợp ăn vào buổi chiều.",
    "Sầu riêng béo thơm, đúng kiểu mình thích.",
    "Bơ xanh mịn, vị lạ mà ngon bất ngờ.",
    "Oreo có chút giòn vui miệng, rất hợp với kem.",
    "Khoai môn thơm và màu tím nhìn cực xinh.",
    "Sữa chua chua nhẹ, ăn trời nóng rất hợp.",
    "Rainbow 7 màu đẹp như một chiếc cầu vồng nhỏ.",
    "Chủ tiệm dễ thương, giao kem đúng món.",
    "Quầy nhỏ nhưng làm kem nhìn rất có tâm.",
    "Mình sẽ quay lại thử thêm những vị khác.",
    "Kem được múc đầy đặn, nhìn là thấy vui.",
    "Phục vụ nhanh gọn, mình không phải chờ lâu.",
    "Không gian pastel nhìn rất thư giãn.",
    "Một viên kem nhỏ mà làm buổi chiều vui hẳn.",
    "Đúng món mình gọi, thao tác rất chuyên nghiệp.",
    "Kem mềm, thơm và không bị đá.",
    "Ốc quế giòn thơm, ăn tới cuối vẫn ngon.",
    "Mình thích cách tiệm chăm chút từng đơn.",
    "5 sao cho một góc kem đáng yêu trong phố.",
    "Vị kem hài hòa, giá cũng dễ chịu.",
    "Được nhìn múc kem nên cảm giác rất thú vị.",
    "Quầy inox sạch sẽ, kem được giữ gọn gàng.",
    "Nhân viên phản hồi nhanh, rất dễ thương.",
    "Kem ngon, tiệm xinh, chắc chắn còn ghé."
  ];

  const ONE_STAR_REVIEWS = [

    "Múc kem kiểu gì lâu thế, mình chờ muốn xỉu!",
    "Bán buôn chán quá, đi về!",
    "Chờ một viên kem mà cảm giác như cả buổi chiều.",
    "Mình hết kiên nhẫn trước khi nhận được món.",
    "Đứng đợi quá lâu nên thôi, hẹn dịp khác.",
    "Hôm nay quầy xử lý đơn chậm quá.",
    "Kem chưa tới mà tâm trạng đã nguội lạnh.",
    "Mình đã gọi đúng món nhưng phải chờ quá lâu.",
    "Chờ lâu quá nên mình bỏ cuộc giữa chừng.",
    "Một viên kem thôi mà sao khó đến vậy?",
    "Tiệm xinh nhưng hôm nay phục vụ chưa ổn.",
    "Mình phải đi vì không thể đợi thêm nữa.",
    "Hy vọng lần sau tốc độ múc kem sẽ nhanh hơn.",
    "Đứng trước quầy lâu quá mà vẫn chưa xong.",
    "Hôm nay mình không có trải nghiệm vui.",
    "Khách đang vội mà đợi thế này thì khó quá.",
    "Mùi kem thơm nhưng chờ lâu quá mất vui.",
    "Xin lỗi shop nhưng hôm nay mình chỉ cho 1 sao.",
    "Đợi tới mức muốn về nhà tự làm kem luôn.",
    "Quầy cần xử lý đơn nhanh hơn một chút nha.",
    "Mình đã hết kiên nhẫn với đơn này.",
    "Chờ héo cả người vẫn chưa có kem.",
    "Đáng lẽ chỉ vài giây mà thành quá lâu.",
    "Hôm nay tiệm chưa phục vụ tốt như mong đợi.",
    "Mình bỏ về vì không muốn đứng đợi nữa.",
    "Chờ lâu làm mất hết hứng ăn kem.",
    "Hy vọng shop nâng tốc độ phục vụ.",
    "Một đơn đơn giản mà xử lý quá chậm.",
    "Mình sẽ cân nhắc quay lại nếu phục vụ nhanh hơn.",
    "Đứng chờ lâu quá, lần này thật sự thất vọng."
  ];


  /* =========================================================
     STATE
  ========================================================= */

  const createInventory = () => {

    const result = {};

    FLAVORS.forEach(
      flavor => {
        result[flavor.id] = 6;
      }
    );

    return result;
  };


  const defaultState = () => ({

    money: 300,

    day: 1,

    open: false,

    seconds: 120,

    capacity: 12,

    scoopTime: 600,

    staffLevel: 0,

    freezerLevel: 0,

    decorLevel: 0,

    inventory: createInventory(),

    currentCustomer: null,

    order: [],

    scooped: [],

    daily: {
      revenue: 0,
      cost: 0,
      success: 0,
      failed: 0
    },

    reviews: createInitialReviews()

  });


  function loadState() {

    try {

      const saved =
        localStorage.getItem(
          "tiemKemCozySave"
        );

      if (!saved) {
        return defaultState();
      }

      const parsed =
        JSON.parse(saved);

      const fresh =
        defaultState();

      return {
        ...fresh,
        ...parsed,
        open: false,
        currentCustomer: null,
        order: [],
        scooped: []
      };

    } catch {

      return defaultState();

    }

  }


  const state = loadState();


  function saveState() {

    try {

      const safeState = {
        ...state,

        open: false,

        currentCustomer: null,

        order: [],

        scooped: []

      };

      localStorage.setItem(
        "tiemKemCozySave",
        JSON.stringify(safeState)
      );

    } catch {}

  }


  /* =========================================================
     INITIAL REVIEWS
  ========================================================= */

  function createInitialReviews() {

    return FIVE_STAR_REVIEWS
      .slice(0, 12)
      .map(
        (text, index) => ({
          name:
            CUSTOMER_NAMES[index],
          avatar:
            AVATARS[index % AVATARS.length],
          stars: 5,
          text,
          time:
            `${index + 1} ngày trước`
        })
      );

  }


  /* =========================================================
     RUNTIME
  ========================================================= */

  let dayInterval = null;
  let customerInterval = null;
  let customerTimer = null;
  let toastTimeout = null;
  let reviewFilter = "all";


  /* =========================================================
     FIND FLAVOR
  ========================================================= */

  function getFlavor(id) {

    return FLAVORS.find(
      flavor => flavor.id === id
    );

  }


  /* =========================================================
     TOAST
  ========================================================= */

  function showToast(message) {

    const toast = $("toast");

    toast.textContent = message;

    toast.classList.add("show");

    clearTimeout(toastTimeout);

    toastTimeout =
      setTimeout(
        () => {
          toast.classList.remove("show");
        },
        2000
      );

  }


  /* =========================================================
     RENDER EVERYTHING
  ========================================================= */

  function renderAll() {

    renderHeader();

    renderCounterTubs();

    renderFlavorGrid();

    renderOrder();

    renderCustomer();

    renderWarehouse();

    renderUpgrades();

    renderReviews();

    renderIceCream();

  }


  /* =========================================================
     HEADER
  ========================================================= */

  function renderHeader() {

    $("money").textContent =
      moneyFormatter.format(
        Math.round(state.money)
      );

    $("dayNumber").textContent =
      state.day;

    $("warehouseMoney").textContent =
      money(state.money);

    $("upgradeMoney").textContent =
      money(state.money);

    $("shopStatus").textContent =
      state.open
        ? "Đang mở cửa"
        : "Đang đóng cửa";

    $("shopHint").textContent =
      state.open
        ? "Múc đúng vị và giao thật nhanh nhé!"
        : "Mở cửa để bắt đầu bán kem.";

    $("openShopButton").textContent =
      state.open
        ? "ĐÓNG CỬA TIỆM"
        : "MỞ CỬA TIỆM";

    updateDayClock();

  }


  /* =========================================================
     DAY CLOCK
  ========================================================= */

  function updateDayClock() {

    const minutes =
      Math.floor(
        state.seconds / 60
      );

    const seconds =
      state.seconds % 60;

    $("dayTime").textContent =
      `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;


    const percent =
      state.seconds / 120;

    const circumference =
      2 * Math.PI * 23;

    $("dayProgress").style.strokeDashoffset =
      circumference *
      (1 - percent);


    $("dayProgress").style.stroke =
      percent > .5
        ? "#70ae8f"
        : percent > .25
          ? "#d4b44e"
          : "#d97575";

  }


  /* =========================================================
     COUNTER TUBS
  ========================================================= */

  function renderCounterTubs() {

    const container =
      $("counterTubs");

    container.innerHTML =
      FLAVORS.map(
        flavor => {

          const amount =
            state.inventory[flavor.id] || 0;

          return `
            <button
              class="stainless-tub"
              data-scoop="${flavor.id}"
              style="--flavor:${flavor.color}"
              aria-label="Múc kem ${flavor.name}"
            >

              <div class="stainless-inside">

                <div class="tub-ice"></div>

                <strong>
                  ${flavor.emoji}
                  ${flavor.name}
                </strong>

              </div>

              <span class="tub-quantity">
                ${amount}
              </span>

            </button>
          `;

        }
      ).join("");


    document
      .querySelectorAll("[data-scoop]")
      .forEach(
        button => {

          button.addEventListener(
            "click",
            () => {
              scoopFlavor(
                button.dataset.scoop
              );
            }
          );

        }
      );

  }


  /* =========================================================
     FLAVOR GRID
  ========================================================= */

  function renderFlavorGrid() {

    $("stockSummary").textContent =
      `${Object.values(state.inventory).reduce(
        (a, b) => a + b,
        0
      )}/${
        state.capacity * FLAVORS.length
      }`;


    $("flavorGrid").innerHTML =
      FLAVORS.map(
        flavor => {

          const amount =
            state.inventory[flavor.id] || 0;

          return `
            <button
              class="flavor-button"
              data-flavor="${flavor.id}"
            >

              <div
                class="flavor-art"
                style="--flavor:${flavor.color}"
              ></div>

              <div class="flavor-info">

                <strong>
                  ${flavor.emoji}
                  ${flavor.name}
                </strong>

                <small>
                  ${money(flavor.price)} / viên
                </small>

              </div>

              <span class="flavor-stock">
                ${amount}
              </span>

            </button>
          `;

        }
      ).join("");


    document
      .querySelectorAll("[data-flavor]")
      .forEach(
        button => {

          button.addEventListener(
            "click",
            () => {
              scoopFlavor(
                button.dataset.flavor
              );
            }
          );

        }
      );

  }


  /* =========================================================
     ORDER
  ========================================================= */

  function renderOrder() {

    const customer =
      state.currentCustomer;

    if (!customer) {

      $("orderText").textContent =
        "Chưa có khách";

      $("customerMood").textContent =
        state.open
          ? "🪑 Đang chờ khách"
          : "🏠 Đóng cửa";

      $("orderDots").innerHTML = "";

    } else {

      $("orderText").textContent =
        state.order
          .map(
            id =>
              `1 viên ${getFlavor(id).name}`
          )
          .join(" + ");


      $("orderDots").innerHTML =
        state.order
          .map(
            id => {

              const flavor =
                getFlavor(id);

              return `
                <span
                  class="order-dot"
                  style="--flavor:${flavor.color}"
                ></span>
              `;

            }
          )
          .join("");


      $("customerMood").textContent =
        "😊 Khách đang chờ";

    }


    $("resetIceCream").disabled =
      state.scooped.length === 0;

    $("serveCustomer").disabled =
      !customer ||
      state.scooped.length !==
        state.order.length;

  }


  /* =========================================================
     ICE CREAM ASSEMBLY
  ========================================================= */

  function renderIceCream() {

    const assembly =
      $("iceCreamAssembly");

    assembly
      .querySelectorAll(".assembly-scoop")
      .forEach(
        element =>
          element.remove()
      );


    const empty =
      assembly.querySelector(
        ".empty-assembly"
      );


    if (state.scooped.length === 0) {

      empty.style.display =
        "flex";

      return;

    }


    empty.style.display =
      "none";


    state.scooped.forEach(
      (flavorId, index) => {

        const flavor =
          getFlavor(flavorId);

        const scoop =
          document.createElement(
            "div"
          );

        scoop.className =
          "assembly-scoop";

        scoop.style.setProperty(
          "--flavor",
          flavor.color
        );

        /*
          Viên đầu nằm thấp,
          viên sau nằm cao hơn.
        */

        scoop.style.bottom =
          `${25 + index * 25}px`;

        /*
          Z-index giúp viên sau
          nằm trên viên trước.
        */

        scoop.style.zIndex =
          20 + index;

        assembly.appendChild(
          scoop
        );

      }
    );

  }


  /* =========================================================
     SCOOP MECHANIC
  ========================================================= */

  function scoopFlavor(flavorId) {

    if (!state.open) {

      showToast(
        "Hãy mở cửa tiệm trước nhé! 🍦"
      );

      return;

    }


    if (!state.currentCustomer) {

      showToast(
        "Chưa có khách gọi món."
      );

      return;

    }


    if (
      state.scooped.length >=
      state.order.length
    ) {

      showToast(
        "Chiếc kem đã đủ viên rồi!"
      );

      return;

    }


    const requiredFlavor =
      state.order[
        state.scooped.length
      ];


    if (
      flavorId !== requiredFlavor
    ) {

      showToast(
        `Khách đang gọi kem ${getFlavor(requiredFlavor).name} nhé!`
      );

      return;

    }


    if (
      state.inventory[flavorId] <= 0
    ) {

      showToast(
        "Khay này hết kem. Vào Kho Hàng nhập thêm nhé."
      );

      return;

    }


    /*
      Trừ kho ngay khi múc.
    */

    state.inventory[flavorId]--;

    state.scooped.push(
      flavorId
    );


    /*
      Hiệu ứng múc.
    */

    const assembly =
      $("iceCreamAssembly");

    assembly.classList.remove(
      "scooping-animation"
    );

    void assembly.offsetWidth;

    assembly.classList.add(
      "scooping-animation"
    );


    renderCounterTubs();
    renderFlavorGrid();
    renderOrder();
    renderIceCream();

  }


  /* =========================================================
     RESET ICE CREAM
  ========================================================= */

  function resetIceCream() {

    if (
      state.scooped.length === 0
    ) {

      return;

    }


    /*
      Trả toàn bộ viên đã múc
      về đúng khay.
    */

    state.scooped.forEach(
      flavorId => {

        state.inventory[flavorId] =
          Math.min(
            state.capacity,
            state.inventory[flavorId] + 1
          );

      }
    );


    state.scooped = [];


    renderCounterTubs();
    renderFlavorGrid();
    renderOrder();
    renderIceCream();


    showToast(
      "Đã làm lại chiếc kem. Kem được trả về khay."
    );

  }


  /* =========================================================
     CUSTOMER SVG
  ========================================================= */

  function customerSVG(customer) {

    const angry =
      customer.patience <= 0;


    let hair = "";


    if (
      customer.hair === "long"
    ) {

      hair = `
        <path
          d="
            M22 55
            Q20 18 54 14
            Q88 18 86 55
            L77 84
            L65 69
            L54 81
            L42 69
            L30 84
            Z
          "
          fill="#704a3b"
        />
      `;

    } else if (
      customer.hair === "bob"
    ) {

      hair = `
        <path
          d="
            M23 54
            Q24 17 54 15
            Q84 17 85 54
            L75 72
            Q70 35 54 29
            Q38 35 32 72
            Z
          "
          fill="#694638"
        />
      `;

    } else if (
      customer.hair === "cap"
    ) {

      hair = `
        <path
          d="
            M24 42
            Q52 13 82 39
            L77 48
            Q53 35 28 48
            Z
          "
          fill="#d49cad"
        />

        <path
          d="
            M25 40
            Q52 14 81 39
          "
          fill="none"
          stroke="#a16d83"
          stroke-width="4"
          stroke-linecap="round"
        />
      `;

    } else {

      hair = `
        <path
          d="
            M27 43
            Q30 17 54 15
            Q78 17 81 43
            L69 34
            Q60 28 54 28
            Q44 28 34 35
            Z
          "
          fill="#604035"
        />
      `;

    }


    const mouth =
      angry
        ? `
          <path
            d="M44 70 Q54 62 64 70"
            fill="none"
            stroke="#a34f4f"
            stroke-width="2.5"
            stroke-linecap="round"
          />
        `
        : `
          <path
            d="M45 67 Q54 74 63 67"
            fill="none"
            stroke="#8d5b55"
            stroke-width="2.5"
            stroke-linecap="round"
          />
        `;


    return `
      <svg
        class="customer-svg ${angry ? "angry" : ""}"
        viewBox="0 0 110 153"
        aria-hidden="true"
      >

        <!-- áo -->

        <path
          d="
            M27 102
            Q55 89 83 102
            L96 153
            L14 153
            Z
          "
          fill="${customer.shirt}"
        />

        <!-- cổ áo -->

        <path
          d="
            M42 99
            Q55 108 68 99
          "
          fill="none"
          stroke="#fff"
          stroke-width="4"
          opacity=".65"
        />

        <!-- mặt -->

        <circle
          class="face"
          cx="55"
          cy="53"
          r="29"
          fill="${customer.skin}"
        />

        <!-- tai -->

        <circle
          cx="27"
          cy="55"
          r="5"
          fill="${customer.skin}"
        />

        <circle
          cx="83"
          cy="55"
          r="5"
          fill="${customer.skin}"
        />

        <!-- tóc -->

        ${hair}

        <!-- mắt -->

        <ellipse
          class="eye"
          cx="44"
          cy="53"
          rx="2.7"
          ry="3"
          fill="#4d403c"
        />

        <ellipse
          class="eye"
          cx="66"
          cy="53"
          rx="2.7"
          ry="3"
          fill="#4d403c"
        />

        <!-- má -->

        <circle
          class="cheek"
          cx="36"
          cy="63"
          r="5"
          fill="#e8898e"
        />

        <circle
          class="cheek"
          cx="74"
          cy="63"
          r="5"
          fill="#e8898e"
        />

        <!-- miệng -->

        ${mouth}

        <!-- tay -->

        <path
          d="
            M28 110
            Q18 118 22 133
          "
          fill="none"
          stroke="${customer.skin}"
          stroke-width="7"
          stroke-linecap="round"
        />

        <path
          d="
            M82 110
            Q92 118 88 133
          "
          fill="none"
          stroke="${customer.skin}"
          stroke-width="7"
          stroke-linecap="round"
        />

      </svg>
    `;

  }


  /* =========================================================
     CUSTOMER RENDER
  ========================================================= */

  function renderCustomer() {

    const layer =
      $("customerLayer");

    if (
      !state.currentCustomer
    ) {

      layer.innerHTML = "";

      return;

    }


    const customer =
      state.currentCustomer;

    const progress =
      Math.max(
        0,
        customer.patience /
          customer.maxPatience
      ) * 100;


    const timerColor =
      progress > 50
        ? "#70ae8f"
        : progress > 25
          ? "#d4b44e"
          : "#d97575";


    let speech = "";


    if (
      customer.patience <= 0
    ) {

      speech =
        customer.complaint;

    } else {

      const first =
        getFlavor(
          state.order[0]
        );

      const second =
        state.order.length > 1
          ? getFlavor(
              state.order[1]
            )
          : null;


      speech =
        second
          ? `Cho mình 1 viên ${first.emoji} ${first.name} + 1 viên ${second.emoji} ${second.name} nhé!`
          : `Cho mình 1 viên ${first.emoji} ${first.name} nhé!`;

    }


    layer.innerHTML = `
      <div
        class="customer"
        id="activeCustomer"
      >

        <div class="customer-bubble">
          ${speech}
        </div>

        <div
          class="customer-timer"
          style="
            --timer-progress:${progress};
            --timer-color:${timerColor};
          "
        >
          <span>
            ${Math.ceil(
              Math.max(
                0,
                customer.patience
              )
            )}
          </span>
        </div>

        ${customerSVG(customer)}

        <div class="customer-name">
          ${customer.name}
        </div>

      </div>
    `;

  }


  /* =========================================================
     CREATE CUSTOMER
  ========================================================= */

  function createCustomer() {

    const available =
      FLAVORS.filter(
        flavor =>
          state.inventory[flavor.id] > 0
      );


    if (
      available.length === 0
    ) {

      showToast(
        "Kho kem đang trống! Hãy nhập hàng."
      );

      return null;

    }


    const first =
      random(available);


    let order = [
      first.id
    ];


    /*
      Khoảng 50% khách gọi 2 viên.
    */

    if (
      Math.random() < .52 &&
      available.length >= 2
    ) {

      const other =
        random(
          available.filter(
            flavor =>
              flavor.id !== first.id
          )
        );

      order.push(
        other.id
      );

    }


    const patience =
      30 +
      state.decorLevel * 4;


    return {

      name:
        random(CUSTOMER_NAMES),

      avatar:
        random(AVATARS),

      shirt:
        random([
          "#e7a9b7",
          "#9ecdbb",
          "#f0ce79",
          "#9eb9df",
          "#c8a5d6"
        ]),

      skin:
        random([
          "#f3c5a0",
          "#dfaa7e",
          "#c98d63",
          "#f6d0ae"
        ]),

      hair:
        random([
          "short",
          "long",
          "bob",
          "cap"
        ]),

      maxPatience:
        patience,

      patience,

      complaint:
        random(
          ONE_STAR_REVIEWS
        )

    };

  }


  /* =========================================================
     START CUSTOMER
  ========================================================= */

  function spawnCustomer() {

    if (
      !state.open ||
      state.currentCustomer
    ) {

      return;

    }


    const customer =
      createCustomer();


    if (!customer) {
      return;
    }


    state.currentCustomer =
      customer;


    state.order =
      createOrderFromCustomer(
        customer
      );


    state.scooped = [];


    renderCustomer();
    renderOrder();
    renderIceCream();


    startCustomerPatience();

  }


  function createOrderFromCustomer() {

    const available =
      FLAVORS.filter(
        flavor =>
          state.inventory[flavor.id] > 0
      );


    if (!available.length) {
      return [];
    }


    const first =
      random(available);


    const result = [
      first.id
    ];


    if (
      Math.random() < .52 &&
      available.length > 1
    ) {

      const remaining =
        available.filter(
          flavor =>
            flavor.id !== first.id
        );

      result.push(
        random(remaining).id
      );

    }


    return result;

  }


  /* =========================================================
     CUSTOMER PATIENCE
  ========================================================= */

  function startCustomerPatience() {

    clearInterval(
      customerTimer
    );


    customerTimer =
      setInterval(
        () => {

          if (
            !state.currentCustomer ||
            !state.open
          ) {

            clearInterval(
              customerTimer
            );

            return;

          }


          state.currentCustomer.patience -=
            .1;


          if (
            state.currentCustomer.patience <= 0
          ) {

            state.currentCustomer.patience = 0;

            failCurrentCustomer();

            return;

          }


          renderCustomer();

        },
        100
      );

  }


  /* =========================================================
     SERVE CUSTOMER
  ========================================================= */

  function serveCustomer() {

    if (
      !state.currentCustomer
    ) {

      return;

    }


    if (
      state.scooped.length !==
      state.order.length
    ) {

      showToast(
        "Chưa đủ các viên kem khách gọi!"
      );

      return;

    }


    /*
      Tính tiền theo từng viên.
    */

    let revenue = 0;


    state.order.forEach(
      flavorId => {

        revenue +=
          getFlavor(flavorId).price;

      }
    );


    state.money +=
      revenue;

    state.daily.revenue +=
      revenue;

    state.daily.success++;


    /*
      Review 5 sao.
    */

    addReview(5);


    /*
      Hiệu ứng khách rời quầy.
    */

    const customerElement =
      $("activeCustomer");


    if (customerElement) {

      customerElement.classList.add(
        "leaving"
      );

    }


    clearInterval(
      customerTimer
    );


    state.currentCustomer = null;
    state.order = [];
    state.scooped = [];


    setTimeout(
      () => {

        renderCustomer();
        renderOrder();
        renderIceCream();

      },
      500
    );


    renderHeader();
    renderCounterTubs();
    renderFlavorGrid();


    showToast(
      `Giao kem thành công! +${money(revenue)} 🍦`
    );


    saveState();


    /*
      Khách tiếp theo.
    */

    setTimeout(
      () => {

        if (state.open) {
          spawnCustomer();
        }

      },
      1000
    );

  }


  /* =========================================================
     FAILED CUSTOMER
  ========================================================= */

  function failCurrentCustomer() {

    clearInterval(
      customerTimer
    );


    if (
      !state.currentCustomer
    ) {

      return;

    }


    state.daily.failed++;


    addReview(1);


    renderCustomer();


    const element =
      $("activeCustomer");


    if (element) {

      element.classList.add(
        "leaving"
      );

    }


    setTimeout(
      () => {

        state.currentCustomer =
          null;

        state.order = [];

        state.scooped = [];


        renderCustomer();
        renderOrder();
        renderIceCream();


        if (
          state.open
        ) {

          setTimeout(
            spawnCustomer,
            850
          );

        }

      },
      800
    );


    saveState();

  }


  /* =========================================================
     ADD REVIEW
  ========================================================= */

  function addReview(stars) {

    const pool =
      stars === 5
        ? FIVE_STAR_REVIEWS
        : ONE_STAR_REVIEWS;


    const used =
      new Set(
        state.reviews.map(
          review =>
            review.text
        )
      );


    const unused =
      pool.filter(
        text =>
          !used.has(text)
      );


    const text =
      random(
        unused.length
          ? unused
          : pool
      );


    state.reviews.unshift({

      name:
        random(CUSTOMER_NAMES),

      avatar:
        random(AVATARS),

      stars,

      text,

      time:
        "vừa xong"

    });


    /*
      Giữ tối đa 100 review.
    */

    state.reviews =
      state.reviews.slice(
        0,
        100
      );

  }


  /* =========================================================
     WAREHOUSE
  ========================================================= */

  function renderWarehouse() {

    $("warehouseList").innerHTML =
      FLAVORS.map(
        flavor => {

          const stock =
            state.inventory[
              flavor.id
            ] || 0;


          const disabled =
            stock >= state.capacity ||
            state.money <
              flavor.wholesale;


          return `
            <article class="warehouse-card">

              <div
                class="warehouse-art"
                style="--flavor:${flavor.color}"
              >
                ${flavor.emoji}
              </div>

              <div>

                <strong>
                  ${flavor.name}
                </strong>

                <small>
                  ${stock}/${state.capacity}
                  viên · +6 viên/thùng
                </small>

              </div>

              <button
                class="secondary-button"
                data-buy="${flavor.id}"
                ${disabled ? "disabled" : ""}
              >
                ${money(flavor.wholesale)}
              </button>

            </article>
          `;

        }
      ).join("");


    document
      .querySelectorAll("[data-buy]")
      .forEach(
        button => {

          button.addEventListener(
            "click",
            () => {

              buyIceCreamBox(
                button.dataset.buy
              );

            }
          );

        }
      );

  }


  /* =========================================================
     BUY ICE CREAM
  ========================================================= */

  function buyIceCreamBox(
    flavorId
  ) {

    const flavor =
      getFlavor(flavorId);


    if (!flavor) {
      return;
    }


    if (
      state.money <
      flavor.wholesale
    ) {

      showToast(
        "Bạn chưa đủ tiền nhập thùng này."
      );

      return;

    }


    if (
      state.inventory[flavorId] >=
      state.capacity
    ) {

      showToast(
        "Khay này đã đầy."
      );

      return;

    }


    state.money -=
      flavor.wholesale;


    state.daily.cost +=
      flavor.wholesale;


    state.inventory[flavorId] =
      Math.min(
        state.capacity,
        state.inventory[flavorId] + 6
      );


    renderAll();

    saveState();


    showToast(
      `Đã nhập thêm 6 viên ${flavor.name}! 📦`
    );

  }


  /* =========================================================
     UPGRADES
  ========================================================= */

  function getUpgradeDefinitions() {

    return [

      {
        id: "speed",

        icon: "🥄",

        name:
          "Muỗng múc nhanh",

        description:
          "Giảm thời gian thao tác múc kem.",

        level:
          Math.round(
            (600 - state.scoopTime) / 70
          ),

        max: 6,

        cost:
          100 +
          Math.round(
            (600 - state.scoopTime) / 70
          ) * 60

      },

      {
        id: "freezer",

        icon: "🧊",

        name:
          "Tủ đông lớn hơn",

        description:
          "Tăng sức chứa mỗi khay kem thêm 4 viên.",

        level:
          state.freezerLevel,

        max: 5,

        cost:
          130 +
          state.freezerLevel * 95

      },

      {
        id: "staff",

        icon: "🧑‍🍳",

        name:
          "Thuê nhân viên",

        description:
          "Nhân viên có thể tự hoàn thành đơn sau một khoảng chờ.",

        level:
          state.staffLevel,

        max: 3,

        cost:
          230 +
          state.staffLevel * 180

      },

      {
        id: "decor",

        icon: "🌷",

        name:
          "Trang trí xe kem",

        description:
          "Khách kiên nhẫn hơn và tiệm trông đáng yêu hơn.",

        level:
          state.decorLevel,

        max: 5,

        cost:
          110 +
          state.decorLevel * 85

      }

    ];

  }


  function renderUpgrades() {

    $("upgradeList").innerHTML =
      getUpgradeDefinitions()
        .map(
          upgrade => {

            const maxed =
              upgrade.level >=
              upgrade.max;


            const disabled =
              maxed ||
              state.money <
                upgrade.cost;


            return `
              <article class="upgrade-card">

                <div class="upgrade-icon">
                  ${upgrade.icon}
                </div>

                <div>

                  <span class="level">
                    CẤP ${upgrade.level}/${upgrade.max}
                  </span>

                  <h3>
                    ${upgrade.name}
                  </h3>

                  <p>
                    ${upgrade.description}
                  </p>

                </div>

                <div class="upgrade-bottom">

                  <span class="level">
                    ${
                      maxed
                        ? "ĐÃ TỐI ĐA"
                        : money(upgrade.cost)
                    }
                  </span>

                  <button
                    class="secondary-button"
                    data-upgrade="${upgrade.id}"
                    ${disabled ? "disabled" : ""}
                  >
                    NÂNG CẤP
                  </button>

                </div>

              </article>
            `;

          }
        )
        .join("");


    document
      .querySelectorAll(
        "[data-upgrade]"
      )
      .forEach(
        button => {

          button.addEventListener(
            "click",
            () => {

              upgradeShop(
                button.dataset.upgrade
              );

            }
          );

        }
      );

  }


  /* =========================================================
     UPGRADE SHOP
  ========================================================= */

  function upgradeShop(
    upgradeId
  ) {

    const upgrade =
      getUpgradeDefinitions()
        .find(
          item =>
            item.id ===
            upgradeId
        );


    if (!upgrade) {
      return;
    }


    if (
      upgrade.level >=
      upgrade.max
    ) {

      return;

    }


    if (
      state.money <
      upgrade.cost
    ) {

      showToast(
        "Chưa đủ tiền nâng cấp."
      );

      return;

    }


    state.money -=
      upgrade.cost;


    switch (upgradeId) {

      case "speed":

        state.scoopTime =
          Math.max(
            180,
            state.scoopTime - 70
          );

        break;


      case "freezer":

        state.freezerLevel++;

        state.capacity += 4;

        break;


      case "staff":

        state.staffLevel++;

        break;


      case "decor":

        state.decorLevel++;

        break;

    }


    renderAll();

    saveState();


    showToast(
      "Nâng cấp thành công! 🌷"
    );

  }


  /* =========================================================
     AUTO STAFF
  ========================================================= */

  function staffLogic() {

    if (
      !state.open ||
      state.staffLevel <= 0 ||
      !state.currentCustomer
    ) {

      return;

    }


    /*
      Nhân viên tự xử lý đơn
      nếu người chơi để khách chờ.
    */

    const delay =
      Math.max(
        3500,
        7000 -
          state.staffLevel * 1100
      );


    setTimeout(
      () => {

        if (
          !state.open ||
          !state.currentCustomer
        ) {

          return;

        }


        /*
          Chỉ tự làm nếu người chơi
          chưa múc viên nào.
        */

        if (
          state.scooped.length > 0
        ) {

          return;

        }


        /*
          Kiểm tra kho.
        */

        const possible =
          state.order.every(
            flavorId =>
              state.inventory[flavorId] > 0
          );


        if (!possible) {

          return;

        }


        /*
          Nhân viên múc toàn bộ.
        */

        state.order.forEach(
          flavorId => {

            state.inventory[
              flavorId
            ]--;

            state.scooped.push(
              flavorId
            );

          }
        );


        renderAll();


        setTimeout(
          () => {

            if (
              state.currentCustomer
            ) {

              serveCustomer();

            }

          },
          350
        );

      },
      delay
    );

  }


  /* =========================================================
     REVIEWS
  ========================================================= */

  function renderReviews() {

    const all =
      state.reviews || [];


    const average =
      all.length
        ? all.reduce(
            (sum, review) =>
              sum + review.stars,
            0
          ) / all.length
        : 0;


    $("averageRating").textContent =
      average.toFixed(1);


    $("reviewCount").textContent =
      `${all.length} đánh giá`;


    let visible =
      all;


    if (
      reviewFilter !==
      "all"
    ) {

      visible =
        all.filter(
          review =>
            review.stars ===
            Number(reviewFilter)
        );

    }


    $("reviewList").innerHTML =
      visible
        .map(
          review => {

            const stars =
              "★".repeat(
                review.stars
              ) +
              "☆".repeat(
                5 - review.stars
              );


            return `
              <article class="review">

                <div class="review-top">

                  <div class="review-user">

                    <div class="review-avatar">
                      ${review.avatar}
                    </div>

                    <div>

                      <div class="review-name">
                        ${review.name}
                      </div>

                      <div class="review-time">
                        ${review.time}
                      </div>

                    </div>

                  </div>

                  <div class="review-stars">
                    ${stars}
                  </div>

                </div>

                <div class="review-text">
                  ${review.text}
                </div>

              </article>
            `;

          }
        )
        .join("");

  }


  /* =========================================================
     OPEN SHOP
  ========================================================= */

  function openShop() {

    if (state.open) {

      closeShop();

      return;

    }


    const transition =
      $("openingTransition");


    transition.classList.add(
      "active"
    );


    /*
      Cho rèm xuất hiện trước.
    */

    setTimeout(
      () => {

        transition.classList.add(
          "play"
        );

      },
      80
    );


    /*
      Sau khoảng 1.5 giây
      mới thực sự bắt đầu ngày.
    */

    setTimeout(
      () => {

        transition.classList.remove(
          "active",
          "play"
        );


        state.open = true;

        state.seconds = 120;

        state.currentCustomer =
          null;

        state.order = [];

        state.scooped = [];


        renderAll();

        startDay();


        setTimeout(
          spawnCustomer,
          500
        );


        /*
          Nhân viên tự động.
        */

        staffLogic();

      },
      1500
    );

  }


  /* =========================================================
     START DAY
  ========================================================= */

  function startDay() {

    clearInterval(
      dayInterval
    );

    clearInterval(
      customerInterval
    );


    dayInterval =
      setInterval(
        () => {

          if (!state.open) {
            return;
          }


          if (
            state.seconds <= 0
          ) {

            closeShop();

            return;

          }


          state.seconds--;

          updateDayClock();


          /*
            Nếu không có khách,
            có cơ hội xuất hiện khách mới.
          */

          if (
            !state.currentCustomer &&
            Math.random() <
              .055 +
              state.staffLevel * .01
          ) {

            spawnCustomer();

          }

        },
        1000
      );


    /*
      Backup spawn timer.
    */

    customerInterval =
      setInterval(
        () => {

          if (
            state.open &&
            !state.currentCustomer
          ) {

            spawnCustomer();

          }

        },
        6500
      );

  }


  /* =========================================================
     CLOSE SHOP
  ========================================================= */

  function closeShop() {

    if (!state.open) {
      return;
    }


    state.open = false;


    clearInterval(
      dayInterval
    );

    clearInterval(
      customerInterval
    );

    clearInterval(
      customerTimer
    );


    state.currentCustomer =
      null;

    state.order = [];

    state.scooped = [];


    renderAll();


    showDailySummary();

    saveState();

  }


  /* =========================================================
     DAILY SUMMARY
  ========================================================= */

  function showDailySummary() {

    const daily =
      state.daily;


    $("summaryDay").textContent =
      state.day;

    $("summaryRevenue").textContent =
      money(daily.revenue);

    $("summaryCost").textContent =
      money(daily.cost);

    $("summaryProfit").textContent =
      money(
        daily.revenue -
        daily.cost
      );

    $("summarySuccess").textContent =
      daily.success;

    $("summaryFail").textContent =
      daily.failed;


    $("dailySummary")
      .classList.add(
        "visible"
      );

  }


  /* =========================================================
     NEXT DAY
  ========================================================= */

  function nextDay() {

    $("dailySummary")
      .classList.remove(
        "visible"
      );


    state.day++;

    state.seconds = 120;

    state.daily = {

      revenue: 0,

      cost: 0,

      success: 0,

      failed: 0

    };


    state.open = false;

    state.currentCustomer =
      null;

    state.order = [];

    state.scooped = [];


    renderAll();

    saveState();

  }


  /* =========================================================
     NAVIGATION
  ========================================================= */

  function switchScreen(
    screenName
  ) {

    document
      .querySelectorAll(
        ".screen"
      )
      .forEach(
        screen => {

          screen.classList.toggle(
            "active",
            screen.id ===
              `screen-${screenName}`
          );

        }
      );


    document
      .querySelectorAll(
        ".nav-item"
      )
      .forEach(
        item => {

          item.classList.toggle(
            "active",
            item.dataset.screen ===
              screenName
          );

        }
      );


    /*
      Khi chuyển tab,
      render lại dữ liệu tương ứng.
    */

    if (
      screenName ===
      "reviews"
    ) {

      renderReviews();

    }

    if (
      screenName ===
      "warehouse"
    ) {

      renderWarehouse();

    }

    if (
      screenName ===
      "upgrades"
    ) {

      renderUpgrades();

    }

  }


  /* =========================================================
     EVENTS
  ========================================================= */

  document
    .querySelectorAll(
      ".nav-item"
    )
    .forEach(
      button => {

        button.addEventListener(
          "click",
          () => {

            switchScreen(
              button.dataset.screen
            );

          }
        );

      }
    );


  document
    .querySelectorAll(
      ".review-filter"
    )
    .forEach(
      button => {

        button.addEventListener(
          "click",
          () => {

            reviewFilter =
              button.dataset.filter;


            document
              .querySelectorAll(
                ".review-filter"
              )
              .forEach(
                item => {

                  item.classList.toggle(
                    "active",
                    item.dataset.filter ===
                      reviewFilter
                  );

                }
              );


            renderReviews();

          }
        );

      }
    );


  $("startGameButton")
    .addEventListener(
      "click",
      () => {

        $("introModal")
          .classList.remove(
            "visible"
          );

      }
    );


  $("openShopButton")
    .addEventListener(
      "click",
      openShop
    );


  $("resetIceCream")
    .addEventListener(
      "click",
      resetIceCream
    );


  $("serveCustomer")
    .addEventListener(
      "click",
      serveCustomer
    );


  $("nextDayButton")
    .addEventListener(
      "click",
      nextDay
    );


  /* =========================================================
     INITIAL
  ========================================================= */

  renderAll();

})();
