(() => {
  'use strict';

  const $ = (id) => document.getElementById(id);

  const moneyFmt = new Intl.NumberFormat('vi-VN');

  const flavors = [
    {
      id:'choco',
      name:'Sô-cô-la',
      emoji:'🍫',
      color:'#7c543f',
      price:24,
      packCost:80
    },
    {
      id:'strawberry',
      name:'Dâu Tây',
      emoji:'🍓',
      color:'#ef9daf',
      price:22,
      packCost:78
    },
    {
      id:'vanilla',
      name:'Vani',
      emoji:'🍦',
      color:'#fff0b7',
      price:20,
      packCost:74
    },
    {
      id:'matcha',
      name:'Matcha',
      emoji:'🍵',
      color:'#a9d39b',
      price:24,
      packCost:82
    },
    {
      id:'butter',
      name:'Bơ',
      emoji:'🥑',
      color:'#b7cd73',
      price:23,
      packCost:78
    },
    {
      id:'durian',
      name:'Sầu Riêng',
      emoji:'🥭',
      color:'#f2dc7e',
      price:26,
      packCost:86
    },
    {
      id:'taro',
      name:'Khoai Môn',
      emoji:'🍠',
      color:'#c8aadf',
      price:23,
      packCost:78
    },
    {
      id:'oreo',
      name:'Oreo',
      emoji:'🍪',
      color:'#8a8a91',
      price:25,
      packCost:84
    },
    {
      id:'yogurt',
      name:'Sữa Chua',
      emoji:'🥛',
      color:'#dce6f3',
      price:21,
      packCost:76
    },
    {
      id:'rainbow',
      name:'7 Màu Rainbow',
      emoji:'🌈',
      color:'#f5c4da',
      price:28,
      packCost:92
    }
  ];

  /* 25 CÂU 5 SAO */
  const positiveReviews = [
    'Kem sô cô la ở đây ngon xỉu, chủ shop múc nhanh nhiệt tình!',
    '10/10 điểm, kem matcha béo ngậy!',
    'Tiệm xinh xắn, giao kem nhanh thoăn thoắt ❤️',
    'Kem dâu thơm và mềm mịn, mình sẽ quay lại!',
    'Vani nhẹ nhàng, ăn một viên là muốn ăn thêm!',
    'Nhân viên thân thiện, quầy sạch sẽ, rất thích.',
    'Kem bơ béo vừa phải, vị dễ ăn và không ngấy.',
    'Sầu riêng thơm nức, múc đầy tay mà vẫn hết veo!',
    'Khoai môn thơm dịu, màu xinh và ăn rất vui miệng.',
    'Oreo giòn giòn, kem mát lạnh, quá hợp buổi chiều!',
    'Sữa chua chua nhẹ, cực hợp trời nóng.',
    'Rainbow 7 màu đẹp mê, chụp ảnh lên hình siêu cute 🌈',
    'Giá hợp lý, phục vụ nhanh, mình cho tiệm trọn 5 sao!',
    'Chủ tiệm nói chuyện dễ thương, kem cũng ngon nữa.',
    'Đợi đúng chút thôi mà nhận được ly kem rất đáng!',
    'Quầy kem nhỏ mà nhiều vị ghê, tha hồ chọn.',
    'Kem mịn, topping vừa xinh, trải nghiệm rất ổn.',
    'Mình thích nhất vị matcha, thơm mà không quá đắng.',
    'Dâu Tây có mùi thơm tự nhiên, ăn rất đã.',
    'Một góc phố thật dễ thương, lần sau mình dẫn bạn tới!',
    'Kem vani ở đây có vị dịu và thơm, đúng gu mình.',
    'Sầu riêng ngon bất ngờ, béo mà vẫn dễ ăn.',
    'Quầy bán hàng gọn gàng, thao tác nhanh gọn.',
    'Đúng món, đúng vị, phục vụ dễ thương, quá ổn áp!'
  ];

  /* 24 CÂU 1 SAO */
  const negativeReviews = [
    'Bắt khách chờ héo cả người, dẹp tiệm đi!',
    'Chậm như rùa, cạch mặt shop này luôn!',
    'Đợi lâu muốn xỉu mà không thấy kem đâu!',
    'Múc kem thôi mà lâu thế, mình về trước đây!',
    'Chờ mãi vẫn chưa có đồ, mất hết kiên nhẫn.',
    'Định ăn kem vui vẻ mà cuối cùng phải bỏ về.',
    'Shop phản hồi chậm quá, lần này mình không vui.',
    'Đứng đợi ở quầy lâu kinh khủng.',
    'Kem chưa tới mà mình đã hết kiên nhẫn rồi.',
    'Xin lỗi nhưng tốc độ phục vụ hôm nay quá chậm.',
    'Chờ một viên kem mà cảm giác như cả buổi chiều.',
    'Bong bóng gọi món vui mà đợi thì không vui chút nào.',
    'Mình thích vị kem nhưng đợi lâu quá nên thôi.',
    'Tiệm dễ thương nhưng hôm nay phục vụ quá chậm.',
    'Khách tới trước mà phải chờ quá lâu.',
    'Mình đành đi về vì không thể đợi thêm nữa.',
    'Hẹn lần sau nếu shop phục vụ nhanh hơn nhé.',
    'Đợi lâu quá, đánh giá lần này chỉ có thể 1 sao.',
    'Không ngờ múc một viên kem lại mất nhiều thời gian vậy.',
    'Hôm nay trải nghiệm chưa ổn, mong tiệm cải thiện tốc độ.',
    'Đứng từ ngoài vào tới quầy rồi mà vẫn phải đợi dài.',
    'Mùi kem thơm nhưng tâm trạng thì không còn thơm nữa 😅',
    'Mình bỏ cuộc giữa chừng vì chờ quá lâu.',
    'Tiệm cần xử lý đơn nhanh hơn một chút nha.'
  ];

  /* 40+ TÊN KHÁCH */
  const customerNames = [
    'Hoàng Nam',
    'Thu Trang',
    'Bảo Anh',
    'Ngọc Linh',
    'Minh Đức',
    'Mai Phương',
    'Tuấn Kiệt',
    'Khánh An',
    'Gia Hân',
    'Quang Huy',
    'Thanh Tùng',
    'Hà My',
    'Đức Anh',
    'Phương Nhi',
    'Kim Ngân',
    'Anh Khoa',
    'Yến Nhi',
    'Minh Khang',
    'Lan Chi',
    'Trọng Nghĩa',
    'Nhật Minh',
    'Thảo Vy',
    'Hữu Phước',
    'Mỹ Duyên',
    'Quỳnh Anh',
    'Đình Phong',
    'Ngọc Hân',
    'Tấn Phát',
    'Hoài An',
    'Bích Ngọc',
    'Đăng Khoa',
    'Thiên Kim',
    'Mộc Miên',
    'Khôi Nguyên',
    'Tú Uyên',
    'An Nhiên',
    'Đình Quân',
    'Thanh Vy',
    'Khánh Duy',
    'Uyên Nhi'
  ];

  const avatars = [
    '🧑🏻',
    '👩🏻',
    '🧑🏼',
    '👩🏼',
    '🧑🏽',
    '👨🏻',
    '👩🏽',
    '🧑🏾',
    '👨🏽',
    '👩🏾',
    '👧🏻',
    '👦🏻'
  ];

  const hairTypes = [
    'short',
    'long',
    'curly',
    'bob',
    'cap'
  ];

  const shirts = [
    '#f4a9b8',
    '#9ed2c0',
    '#f2cf7b',
    '#a8bde5',
    '#d5a9df',
    '#f2b38a'
  ];

  const skinTones = [
    '#f4c7a1',
    '#e4ae7e',
    '#c88d62',
    '#f7d3b1'
  ];

  const defaultState = () => ({
    money:300,
    day:1,
    isOpen:false,
    daySeconds:120,

    maxStock:12,

    scoopSpeed:850,
    freezerLevel:0,
    staffLevel:0,
    decorLevel:0,

    inventory:Object.fromEntries(
      flavors.map((f) => [f.id,6])
    ),

    currentCustomer:null,
    currentOrder:null,
    scooped:false,
    scoopedFlavor:null,

    customerTimer:null,
    shopTimer:null,

    staffBusy:false,

    daily:{
      revenue:0,
      supplyCost:0,
      success:0,
      fail:0
    },

    reviews:createBaseReviews()
  });

  let state = loadState();

  let toastTimer = null;
  let reviewFilter = 'all';

  function createBaseReviews(){
    const list = [];

    for(let i = 0; i < 10; i++){
      list.push({
        id:`base-${i}`,
        name:customerNames[i],
        avatar:avatars[i % avatars.length],
        stars:5,
        text:positiveReviews[i],
        time:`${i+1} ngày trước`
      });
    }

    return list;
  }

  function saveState(){
    const safe = {
      ...state
    };

    delete safe.customerTimer;
    delete safe.shopTimer;

    localStorage.setItem(
      'sweetIceCreamShop_v1',
      JSON.stringify(safe)
    );
  }

  function loadState(){
    try{
      const raw =
        localStorage.getItem(
          'sweetIceCreamShop_v1'
        );

      if(!raw){
        return defaultState();
      }

      const parsed = JSON.parse(raw);

      const base = defaultState();

      const merged = {
        ...base,
        ...parsed,
        daily:{
          ...base.daily,
          ...(parsed.daily || {})
        }
      };

      merged.inventory = {
        ...base.inventory,
        ...(parsed.inventory || {})
      };

      merged.currentCustomer = null;
      merged.currentOrder = null;
      merged.scooped = false;
      merged.scoopedFlavor = null;
      merged.customerTimer = null;
      merged.shopTimer = null;
      merged.staffBusy = false;

      return merged;
    }catch{
      return defaultState();
    }
  }

  function money(n){
    return `${moneyFmt.format(
      Math.max(0,Math.round(n))
    )}đ`;
  }

  function getFlavor(id){
    return flavors.find(
      f => f.id === id
    );
  }

  function rand(arr){
    return arr[
      Math.floor(
        Math.random() * arr.length
      )
    ];
  }

  function clamp(n,a,b){
    return Math.min(
      b,
      Math.max(a,n)
    );
  }

  function showToast(msg){
    const el = $('toast');

    el.textContent = msg;

    el.classList.add('show');

    clearTimeout(toastTimer);

    toastTimer = setTimeout(
      () => el.classList.remove('show'),
      2200
    );
  }

  function updateTopStats(){
    $('moneyValue').textContent =
      moneyFmt.format(
        Math.round(state.money)
      );

    $('dayValue').textContent =
      state.day;

    $('warehouseMoney').textContent =
      moneyFmt.format(
        Math.round(state.money)
      );

    $('upgradeMoney').textContent =
      moneyFmt.format(
        Math.round(state.money)
      );

    $('maxStockValue').textContent =
      state.maxStock;

    $('openCloseBtn').textContent =
      state.isOpen
        ? 'ĐÓNG CỬA'
        : 'MỞ CỬA TIỆM';

    $('shopStatusText').textContent =
      state.isOpen
        ? 'Tiệm đang mở cửa — phục vụ khách thật nhanh nhé!'
        : 'Tiệm đang đóng cửa. Hãy mở cửa để đón khách!';
  }

  /* =========================
     TUB GRID
  ========================= */

  function renderTubs(){
    const grid = $('tubGrid');

    grid.innerHTML =
      flavors.map(f => {
        const qty =
          state.inventory[f.id] || 0;

        return `
          <button
            class="tub ${qty === 0 ? 'tub-empty' : ''}"
            data-flavor="${f.id}"
            style="--lid:${f.color}"
          >
            <div
              class="tub-lid"
              style="background:${f.color}"
            >
              ${f.emoji}
            </div>

            <div
              class="scoop-dot"
              style="background:${f.color}"
            ></div>

            <div class="tub-info">
              <div class="tub-name">
                Kem ${f.name}
              </div>

              <div class="tub-stock">
                ${qty}/${state.maxStock}
              </div>
            </div>
          </button>
        `;
      }).join('');

    grid
      .querySelectorAll('.tub')
      .forEach(btn => {
        btn.addEventListener(
          'click',
          () => scoopFlavor(
            btn.dataset.flavor
          )
        );
      });
  }

  /* =========================
     WAREHOUSE
  ========================= */

  function renderWarehouse(){
    $('warehouseGrid').innerHTML =
      flavors.map(f => {
        const qty =
          state.inventory[f.id] || 0;

        const next =
          Math.min(
            state.maxStock,
            qty + 6
          );

        const disabled =
          qty >= state.maxStock
            ? 'disabled'
            : '';

        return `
          <div class="warehouse-card">
            <div
              class="warehouse-visual"
              style="background:${f.color}44"
            >
              ${f.emoji}
            </div>

            <h3>${f.name}</h3>

            <p>
              Thùng sỉ +6 viên ·
              Đang có ${qty}/${state.maxStock}
            </p>

            <div class="pack-price">
              <span>${money(f.packCost)}</span>

              <button
                class="btn btn-secondary buy-btn"
                ${disabled}
                data-pack="${f.id}"
              >
                NHẬP THÙNG
              </button>
            </div>

            <div class="upgrade-note">
              Sau nhập:
              ${next}/${state.maxStock}
            </div>
          </div>
        `;
      }).join('');

    $('warehouseGrid')
      .querySelectorAll('[data-pack]')
      .forEach(btn => {
        btn.addEventListener(
          'click',
          () => buyPack(
            btn.dataset.pack
          )
        );
      });
  }

  /* =========================
     UPGRADES
  ========================= */

  function upgradeData(){
    return [
      {
        id:'speed',
        icon:'🥄',
        title:'Muỗng múc nhanh',
        desc:'Giảm thời gian thao tác múc kem. Càng nâng càng nhanh.',
        level:Math.round(
          (1200 - state.scoopSpeed) / 120
        ),
        max:8,
        cost:
          90 +
          Math.round(
            (1200 - state.scoopSpeed) / 120
          ) * 45
      },

      {
        id:'freezer',
        icon:'🧊',
        title:'Tủ đông xịn',
        desc:'Tăng sức chứa mỗi vị thêm 4 viên.',
        level:state.freezerLevel,
        max:5,
        cost:
          120 +
          state.freezerLevel * 90
      },

      {
        id:'staff',
        icon:'🧑‍🍳',
        title:'Thuê nhân viên',
        desc:'Nhân viên tự phục vụ khách khi tiệm đang mở.',
        level:state.staffLevel,
        max:3,
        cost:
          220 +
          state.staffLevel * 170
      },

      {
        id:'decor',
        icon:'🌷',
        title:'Trang trí tiệm',
        desc:'Tăng kiên nhẫn khách và giúp tiệm trông vui mắt hơn.',
        level:state.decorLevel,
        max:5,
        cost:
          100 +
          state.decorLevel * 80
      }
    ];
  }

  function renderUpgrades(){
    $('upgradeGrid').innerHTML =
      upgradeData().map(u => {
        const full =
          u.level >= u.max;

        return `
          <div class="upgrade-card">
            <div class="upgrade-icon">
              ${u.icon}
            </div>

            <div>
              <span class="lvl">
                CẤP ${u.level}/${u.max}
              </span>

              <h3>${u.title}</h3>

              <p>${u.desc}</p>
            </div>

            <div class="wide upgrade-buy">
              <span class="upgrade-note">
                ${
                  full
                    ? 'Đã đạt tối đa'
                    : `Giá ${money(u.cost)}`
                }
              </span>

              <button
                class="btn btn-secondary buy-btn"
                data-upgrade="${u.id}"
                ${full ? 'disabled' : ''}
              >
                NÂNG CẤP
              </button>
            </div>
          </div>
        `;
      }).join('');

    $('upgradeGrid')
      .querySelectorAll('[data-upgrade]')
      .forEach(btn => {
        btn.addEventListener(
          'click',
          () => buyUpgrade(
            btn.dataset.upgrade
          )
        );
      });
  }

  /* =========================
     REVIEWS
  ========================= */

  function renderReviews(){
    const source =
      state.reviews || [];

    const filtered =
      reviewFilter === 'all'
        ? source
        : source.filter(
            r =>
              String(r.stars)
              === reviewFilter
          );

    const avg =
      source.length
        ? source.reduce(
            (s,r) => s + r.stars,
            0
          ) / source.length
        : 5;

    $('avgRating').textContent =
      avg.toFixed(1);

    $('avgStars').textContent =
      avg >= 4.5
        ? '★★★★★'
        : avg >= 3
          ? '★★★★☆'
          : '★☆☆☆☆';

    $('reviewCount').textContent =
      `${source.length} đánh giá`;

    $('reviewList').innerHTML =
      filtered.length
        ? filtered.map(r => `
            <article class="review-card">
              <div class="review-top">
                <div class="review-user">
                  <div class="avatar">
                    ${r.avatar}
                  </div>

                  <div>
                    <div class="review-name">
                      ${escapeHtml(r.name)}
                    </div>

                    <div class="review-time">
                      ${escapeHtml(r.time)}
                    </div>
                  </div>
                </div>

                <div
                  class="
                    stars
                    ${r.stars === 5
                      ? 'star-good'
                      : 'star-bad'}
                  "
                >
                  ${
                    '★'.repeat(r.stars)
                  }${
                    '☆'.repeat(
                      5-r.stars
                    )
                  }
                </div>
              </div>

              <div class="review-text">
                ${escapeHtml(r.text)}
              </div>
            </article>
          `).join('')
        : `
          <div class="hint-card">
            Chưa có đánh giá phù hợp.
          </div>
        `;

    document
      .querySelectorAll('.filter-btn')
      .forEach(btn => {
        btn.classList.toggle(
          'active',
          btn.dataset.filter
            === reviewFilter
        );
      });
  }

  function escapeHtml(str){
    return String(str).replace(
      /[&<>'"]/g,
      ch => ({
        '&':'&amp;',
        '<':'&lt;',
        '>':'&gt;',
        "'":'&#39;',
        '"':'&quot;'
      }[ch])
    );
  }

  /* =========================
     TABS
  ========================= */

  function switchTab(tabId){
    document
      .querySelectorAll('.tab-panel')
      .forEach(p =>
        p.classList.remove('active')
      );

    $(tabId)
      .classList.add('active');

    document
      .querySelectorAll('.nav-btn')
      .forEach(b =>
        b.classList.toggle(
          'active',
          b.dataset.tab === tabId
        )
      );

    window.scrollTo({
      top:0,
      behavior:'smooth'
    });
  }

  /* =========================
     CONE
  ========================= */

  function resetCone(){
    state.scooped = false;
    state.scoopedFlavor = null;

    $('serveBtn').disabled = true;
    $('resetScoopBtn').disabled = true;

    $('coneDisplay').innerHTML = `
      <div class="cone-placeholder">
        🍦
        <span>
          Chạm HŨ KEM<br>
          để múc
        </span>
      </div>
    `;
  }

  function drawCone(flavor){
    $('coneDisplay').innerHTML = `
      <div class="cone"></div>

      <div
        class="scoop"
        style="
          background:${flavor.color};
          box-shadow:none
        "
      ></div>
    `;
  }

  /* =========================
     CUSTOMER
  ========================= */

  function currentPatienceMax(){
    return 30 + state.decorLevel * 3;
  }

  function createCustomer(){
    if(
      !state.isOpen ||
      state.currentCustomer
    ){
      return;
    }

    const flavor = rand(flavors);

    const gender =
      Math.random() > .5
        ? 'female'
        : 'male';

    const profile = {
      id:
        Date.now() +
        Math.random(),

      name:
        rand(customerNames),

      avatar:
        rand(avatars),

      gender,

      hair:
        rand(hairTypes),

      shirt:
        rand(shirts),

      skin:
        rand(skinTones),

      flavorId:
        flavor.id,

      patience:
        currentPatienceMax(),

      maxPatience:
        currentPatienceMax(),

      phase:'entering'
    };

    state.currentCustomer =
      profile;

    state.currentOrder =
      flavor.id;

    resetCone();

    renderCustomer();

    updateOrderUI();

    startCustomerTimer();

    setTimeout(() => {
      if(
        state.currentCustomer?.id
        === profile.id
      ){
        profile.phase = 'ready';

        scheduleStaffService();
      }
    },900);
  }

  function buildCustomerSvg(
    c,
    angry = false
  ){
    const hair =
      c.hair === 'cap'
        ? `
          <path
            d="
              M27 35
              Q52 10 77 35
              L74 46
              Q51 31 30 46Z
            "
            fill="#e1b8d2"
          />

          <path
            d="M26 34 Q52 10 78 34"
            fill="none"
            stroke="#9d7591"
            stroke-width="4"
            stroke-linecap="round"
          />
        `
        : c.hair === 'long'
          ? `
            <path
              d="
                M24 47
                Q20 15 52 15
                Q84 15 80 47
                L73 79
                L63 68
                L58 78
                L45 68
                L37 80Z
              "
              fill="#6e4a3c"
            />
          `
          : c.hair === 'curly'
            ? `
              <g fill="#6d4b3b">
                <circle cx="31" cy="31" r="10"/>
                <circle cx="40" cy="21" r="11"/>
                <circle cx="53" cy="18" r="12"/>
                <circle cx="66" cy="23" r="11"/>
                <circle cx="75" cy="34" r="10"/>
                <circle cx="33" cy="43" r="9"/>
                <circle cx="73" cy="44" r="9"/>
              </g>
            `
            : c.hair === 'bob'
              ? `
                <path
                  d="
                    M23 48
                    Q23 17 52 14
                    Q80 18 82 48
                    L75 63
                    Q67 29 52 30
                    Q37 30 30 63Z
                  "
                  fill="#704b3c"
                />
              `
              : `
                <path
                  d="
                    M27 44
                    Q28 16 52 14
                    Q77 16 78 44
                    L70 35
                    Q60 27 52 27
                    Q43 27 34 35Z
                  "
                  fill="#684639"
                />
              `;

    const shirt = `
      <path
        d="
          M30 88
          Q52 77 74 88
          L84 132
          L20 132Z
        "
        fill="${c.shirt}"
      />
    `;

    const eyes = `
      <g class="eye-group">
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
      </g>
    `;

    const cheeks = `
      <circle
        cx="37"
        cy="60"
        r="5"
        fill="#ee9b9b"
        opacity=".7"
      />

      <circle
        cx="69"
        cy="60"
        r="5"
        fill="#ee9b9b"
        opacity=".7"
      />
    `;

    const skin =
      angry
        ? '#ef9c96'
        : c.skin;

    const mouth =
      angry
        ? `
          <path
            d="M45 68 Q52 62 59 68"
            stroke="#7c3434"
            stroke-width="2.4"
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
          d="M22 94 Q52 79 82 94"
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

  function requestText(c){
    const f =
      getFlavor(c.flavorId);

    const variants =
      c.gender === 'female'
        ? [
            `Dạ shop múc cho em 1 kem ${f.name.toLowerCase()} với ạ!`,
            `Cho chị 1 ly kem ${f.name.toLowerCase()} nhen shop!`,
            `Shop ơi em lấy 1 kem ${f.name.toLowerCase()} nha!`
          ]
        : [
            `Anh lấy 1 kem ${f.name.toLowerCase()} mát lạnh nhé!`,
            `Shop cho anh 1 kem ${f.name.toLowerCase()} với!`,
            `Cho mình 1 viên ${f.name.toLowerCase()} nha shop!`
          ];

    return variants[
      (
        c.name.charCodeAt(0)
        + state.day
      ) % variants.length
    ];
  }

  function renderCustomer(
    angry = false
  ){
    const stage =
      $('customerStage');

    if(!state.currentCustomer){
      stage.innerHTML = '';
      return;
    }

    const c =
      state.currentCustomer;

    const pct =
      clamp(
        (
          c.patience /
          c.maxPatience
        ) * 100,
        0,
        100
      );

    const ringClass =
      pct > 60
        ? ''
        : pct > 30
          ? 'warn'
          : 'danger';

    const text =
      angry
        ? rand(
            negativeReviews.slice(
              0,
              7
            )
          )
        : requestText(c);

    stage.innerHTML = `
      <div
        class="
          customer-card
          ready
          ${angry ? 'leaving' : ''}
        "
        data-customer="1"
      >
        <div class="patience-wrap">
          <div
            class="
              patience-ring
              ${ringClass}
            "
            style="--p:${pct}"
          >
            <span>
              ${Math.ceil(c.patience)}
            </span>
          </div>
        </div>

        <div class="customer-bubble">
          ${escapeHtml(text)}
        </div>

        ${buildCustomerSvg(
          c,
          angry
        )}

        <div class="customer-name-tag">
          ${escapeHtml(c.name)}
        </div>
      </div>
    `;

    stage
      .querySelector(
        '.customer-card'
      )
      ?.addEventListener(
        'click',
        deliverCustomer
      );
  }

  function updateCustomerVisual(){
    const c =
      state.currentCustomer;

    if(!c) return;

    const card =
      $('.customer-card');

    const ring =
      $('.patience-ring');

    const bubble =
      $('.customer-bubble');

    if(
      !card ||
      !ring ||
      !bubble
    ){
      return;
    }

    const pct =
      clamp(
        (
          c.patience /
          c.maxPatience
        ) * 100,
        0,
        100
      );

    ring.classList.toggle(
      'warn',
      pct <= 60 &&
      pct > 30
    );

    ring.classList.toggle(
      'danger',
      pct <= 30
    );

    ring.style.setProperty(
      '--p',
      pct
    );

    const number =
      ring.querySelector('span');

    if(number){
      number.textContent =
        Math.ceil(
          c.patience
        );
    }

    bubble.textContent =
      requestText(c);
  }

  /* =========================
     ORDER UI
  ========================= */

  function updateOrderUI(){
    const request =
      $('customerRequest');

    const mood =
      $('customerMoodTag');

    if(!state.currentCustomer){
      request.textContent =
        state.isOpen
          ? 'Đang chờ khách tiếp theo...'
          : 'Chưa có khách. Mở cửa để bắt đầu.';

      mood.textContent =
        state.isOpen
          ? '🧍 Đang chờ'
          : '🏠 Đóng cửa';

      mood.className =
        'mood-tag';

      $('orderProgressBar')
        .style.width = '0%';

      return;
    }

    const f =
      getFlavor(
        state.currentOrder
      );

    request.textContent =
      `${state.currentCustomer.name}: 1 viên kem ${f.name}`;

    const pct =
      clamp(
        (
          state.currentCustomer.patience /
          state.currentCustomer.maxPatience
        ) * 100,
        0,
        100
      );

    $('orderProgressBar')
      .style.width =
      `${pct}%`;

    mood.textContent =
      pct > 60
        ? '😊 Khách vui'
        : pct > 30
          ? '😬 Hơi sốt ruột'
          : '😡 Sắp nổi giận';

    mood.className =
      'mood-tag ' +
      (
        pct > 60
          ? 'good'
          : pct > 30
            ? 'warn'
            : 'bad'
      );
  }

  /* =========================
     CUSTOMER TIMER
  ========================= */

  function startCustomerTimer(){
    clearInterval(
      state.customerTimer
    );

    state.customerTimer =
      setInterval(() => {
        if(
          !state.isOpen ||
          !state.currentCustomer
        ){
          return;
        }

        const c =
          state.currentCustomer;

        c.patience =
          Math.max(
            0,
            c.patience - 1
          );

        updateCustomerVisual();
        updateOrderUI();

        if(
          c.patience <= 0
        ){
          customerTimeout();
        }
      },1000);
  }

  /* =========================
     CUSTOMER FAIL
  ========================= */

  function customerTimeout(){
    if(
      !state.currentCustomer
    ){
      return;
    }

    clearInterval(
      state.customerTimer
    );

    const c =
      state.currentCustomer;

    state.daily.fail += 1;

    pushReview(
      1,
      rand(negativeReviews)
    );

    renderCustomer(true);

    showToast(
      `${c.name}: “${
        rand(
          negativeReviews.slice(
            0,
            5
          )
        )
      }” 😡`
    );

    state.currentCustomer = null;
    state.currentOrder = null;

    resetCone();
    updateOrderUI();

    saveState();

    setTimeout(() => {
      $('customerStage')
        .innerHTML = '';

      if(state.isOpen){
        setTimeout(
          createCustomer,
          700
        );
      }
    },800);
  }

  /* =========================
     SCOOP
  ========================= */

  function scoopFlavor(id){
    if(
      !state.isOpen ||
      !state.currentCustomer
    ){
      showToast(
        'Mở cửa và đón khách trước nhé!'
      );
      return;
    }

    if(state.scooped){
      showToast(
        'Mẻ này đã có kem rồi. Giao cho khách nhé!'
      );
      return;
    }

    const requested =
      state.currentOrder;

    if(id !== requested){
      const btn =
        document.querySelector(
          `.tub[data-flavor="${id}"]`
        );

      btn?.classList.add(
        'mini-shake'
      );

      setTimeout(
        () => btn?.classList.remove(
          'mini-shake'
        ),
        350
      );

      showToast(
        `Khách gọi kem ${
          getFlavor(requested).name
        }, đừng múc nhầm vị nhé!`
      );

      return;
    }

    if(
      (state.inventory[id] || 0)
      <= 0
    ){
      showToast(
        'Hũ này hết kem rồi, vào Kho Hàng nhập thêm nhé!'
      );
      return;
    }

    const scoopBtn =
      document.querySelector(
        `.tub[data-flavor="${id}"]`
      );

    scoopBtn?.classList.add(
      'mini-shake'
    );

    setTimeout(
      () =>
        scoopBtn?.classList.remove(
          'mini-shake'
        ),
      320
    );

    setTimeout(() => {
      if(
        !state.currentCustomer ||
        state.currentOrder !== id ||
        state.scooped
      ){
        return;
      }

      state.inventory[id] -= 1;

      state.scooped = true;
      state.scoopedFlavor = id;

      drawCone(
        getFlavor(id)
      );

      $('serveBtn').disabled =
        false;

      $('resetScoopBtn').disabled =
        false;

      renderTubs();
      renderWarehouse();

      saveState();

      showToast(
        `Đã múc 1 viên ${
          getFlavor(id).name
        }! 🍦`
      );
    },state.scoopSpeed);
  }

  function resetScoop(){
    if(!state.scooped){
      return;
    }

    const id =
      state.scoopedFlavor;

    state.inventory[id] =
      (state.inventory[id] || 0) + 1;

    resetCone();

    renderTubs();
    renderWarehouse();

    saveState();

    showToast(
      'Đã bỏ mẻ và trả lại viên kem vào hũ.'
    );
  }

  /* =========================
     DELIVER
  ========================= */

  function deliverCustomer(){
    if(
      !state.currentCustomer
    ){
      return;
    }

    if(!state.scooped){
      showToast(
        'Chưa múc đúng vị kem cho khách!'
      );
      return;
    }

    const c =
      state.currentCustomer;

    const f =
      getFlavor(
        state.scoopedFlavor
      );

    clearInterval(
      state.customerTimer
    );

    const speedBonus =
      1 +
      state.staffLevel * 0.04;

    const patienceBonus =
      c.patience /
      c.maxPatience;

    const payout =
      Math.round(
        f.price *
        (
          1 +
          state.decorLevel * 0.03 +
          patienceBonus * 0.08
        ) *
        speedBonus
      );

    state.money += payout;

    state.daily.revenue +=
      payout;

    state.daily.success += 1;

    pushReview(
      5,
      rand(positiveReviews)
    );

    renderCustomer(true);

    const card =
      $('.customer-card');

    card?.classList.remove(
      'ready'
    );

    showToast(
      `Giao thành công! +${money(payout)} 💰`
    );

    state.currentCustomer = null;
    state.currentOrder = null;

    resetCone();
    updateOrderUI();

    saveState();

    setTimeout(() => {
      $('customerStage').innerHTML = '';

      if(state.isOpen){
        setTimeout(
          createCustomer,
          550
        );
      }
    },800);
  }

  /* =========================
     STAFF
  ========================= */

  function scheduleStaffService(){
    if(
      state.staffLevel <= 0 ||
      !state.isOpen ||
      state.staffBusy ||
      !state.currentCustomer ||
      state.currentCustomer.phase !== 'ready'
    ){
      return;
    }

    state.staffBusy = true;

    setTimeout(() => {
      state.staffBusy = false;

      if(
        state.currentCustomer &&
        !state.scooped &&
        state.isOpen
      ){
        scoopFlavor(
          state.currentOrder
        );

        setTimeout(
          () => {
            if(
              state.currentCustomer &&
              state.scooped
            ){
              deliverCustomer();
            }
          },
          state.scoopSpeed + 450
        );
      }
    },850);
  }

  /* =========================
     WAREHOUSE BUY
  ========================= */

  function buyPack(id){
    const f =
      getFlavor(id);

    const qty =
      state.inventory[id] || 0;

    if(
      qty >= state.maxStock
    ){
      showToast(
        'Hũ này đã đầy!'
      );
      return;
    }

    if(
      state.money < f.packCost
    ){
      showToast(
        'Chưa đủ tiền nhập thùng này.'
      );
      return;
    }

    state.money -=
      f.packCost;

    state.daily.supplyCost +=
      f.packCost;

    state.inventory[id] =
      Math.min(
        state.maxStock,
        qty + 6
      );

    renderAll();
    saveState();

    showToast(
      `Đã nhập 1 thùng kem ${f.name}. +${
        Math.min(
          6,
          state.maxStock - qty
        )
      } viên 📦`
    );
  }

  /* =========================
     UPGRADE BUY
  ========================= */

  function buyUpgrade(id){
    const u =
      upgradeData().find(
        x => x.id === id
      );

    if(!u){
      return;
    }

    if(u.level >= u.max){
      showToast(
        'Nâng cấp này đã đạt tối đa.'
      );
      return;
    }

    if(state.money < u.cost){
      showToast(
        'Chưa đủ tiền cho nâng cấp này.'
      );
      return;
    }

    state.money -= u.cost;

    if(id === 'speed'){
      state.scoopSpeed =
        Math.max(
          280,
          state.scoopSpeed - 120
        );
    }

    if(id === 'freezer'){
      state.freezerLevel += 1;
      state.maxStock += 4;

      flavors.forEach(f => {
        state.inventory[f.id] =
          Math.min(
            state.maxStock,
            (state.inventory[f.id] || 0) + 4
          );
      });
    }

    if(id === 'staff'){
      state.staffLevel += 1;
    }

    if(id === 'decor'){
      state.decorLevel += 1;
    }

    renderAll();
    saveState();

    showToast(
      `Đã nâng cấp ${u.title}! ✨`
    );

    if(state.currentCustomer){
      scheduleStaffService();
    }
  }

  /* =========================
     REVIEW GENERATOR
  ========================= */

  function pushReview(
    stars,
    text
  ){
    const usedTexts =
      new Set(
        (state.reviews || [])
          .map(r => r.text)
      );

    const pool =
      stars === 5
        ? positiveReviews
        : negativeReviews;

    let available =
      pool.filter(
        t => !usedTexts.has(t)
      );

    if(!available.length){
      available = pool;
    }

    const reviewer = {
      id:
        `r-${Date.now()}-${Math.random()}`,

      name:
        rand(customerNames),

      avatar:
        rand(avatars),

      stars,

      text:
        rand(available),

      time:
        'vừa xong'
    };

    state.reviews =
      [reviewer,...(state.reviews || [])]
        .slice(0,80);

    renderReviews();
  }

  /* =========================
     SHOP OPEN/CLOSE
  ========================= */

  function toggleShop(){
    if(state.isOpen){
      closeShop(false);
      return;
    }

    openShop();
  }

  function openShop(){
    if(
      $('introModal')
        .classList
        .contains('visible')
    ){
      return;
    }

    if(
      state.daySeconds <= 0
    ){
      state.daySeconds = 120;
    }

    state.isOpen = true;

    state.daily = {
      revenue:0,
      supplyCost:0,
      success:0,
      fail:0
    };

    const overlay =
      $('openingOverlay');

    overlay.classList.add(
      'active'
    );

    requestAnimationFrame(
      () =>
        requestAnimationFrame(
          () =>
            overlay.classList.add(
              'play'
            )
        )
    );

    updateTopStats();
    updateOrderUI();
    saveState();

    setTimeout(
      () => createCustomer(),
      900
    );

    setTimeout(
      () => {
        overlay.classList.remove(
          'play'
        );

        overlay.classList.remove(
          'active'
        );
      },
      1500
    );

    if(!state.shopTimer){
      startDayClock();
    }
  }

  function startDayClock(){
    clearInterval(
      state.shopTimer
    );

    state.shopTimer =
      setInterval(() => {
        if(!state.isOpen){
          return;
        }

        state.daySeconds =
          Math.max(
            0,
            state.daySeconds - 1
          );

        updateDayClockUI();

        if(
          state.daySeconds <= 0
        ){
          closeShop(true);
        }
      },1000);
  }

  function updateDayClockUI(){
    const sec =
      Math.max(
        0,
        state.daySeconds
      );

    const mm =
      String(
        Math.floor(sec / 60)
      ).padStart(2,'0');

    const ss =
      String(
        sec % 60
      ).padStart(2,'0');

    $('dayTimer').textContent =
      `${mm}:${ss}`;

    const ring =
      $('.day-clock-ring');

    ring.classList.toggle(
      'warning',
      sec <= 45 &&
      sec > 20
    );

    ring.classList.toggle(
      'danger',
      sec <= 20
    );
  }

  /* =========================
     CLOSE + SUMMARY
  ========================= */

  function closeShop(auto){
    if(!state.isOpen){
      return;
    }

    state.isOpen = false;

    clearInterval(
      state.customerTimer
    );

    clearInterval(
      state.shopTimer
    );

    state.shopTimer = null;
    state.customerTimer = null;
    state.staffBusy = false;

    state.currentCustomer = null;
    state.currentOrder = null;

    resetCone();

    $('customerStage').innerHTML = '';

    state.daySeconds = 120;

    updateDayClockUI();
    updateOrderUI();
    updateTopStats();

    renderSummary();

    $('daySummaryModal')
      .classList
      .add('visible');

    saveState();

    if(auto){
      showToast(
        'Hết giờ kinh doanh! Đây là tổng kết hôm nay.'
      );
    }
  }

  function renderSummary(){
    $('summaryDay').textContent =
      state.day;

    $('summaryRevenue').textContent =
      money(
        state.daily.revenue
      );

    $('summarySupplyCost').textContent =
      money(
        state.daily.supplyCost
      );

    $('summaryProfit').textContent =
      money(
        state.daily.revenue -
        state.daily.supplyCost
      );

    $('summarySuccess').textContent =
      state.daily.success;

    $('summaryFail').textContent =
      state.daily.fail;

    $('summaryProfit').style.color =
      (
        state.daily.revenue -
        state.daily.supplyCost
      ) >= 0
        ? 'var(--green)'
        : 'var(--red)';
  }

  /* =========================
     NEW DAY
  ========================= */

  function newDay(){
    state.day += 1;

    state.daySeconds = 120;

    state.daily = {
      revenue:0,
      supplyCost:0,
      success:0,
      fail:0
    };

    $('daySummaryModal')
      .classList
      .remove('visible');

    state.isOpen = false;

    updateTopStats();
    updateDayClockUI();
    updateOrderUI();

    showToast(
      `Ngày ${state.day} đã sẵn sàng. Mở cửa thôi! 🌼`
    );

    saveState();
  }

  /* =========================
     RENDER ALL
  ========================= */

  function renderAll(){
    updateTopStats();
    renderTubs();
    renderWarehouse();
    renderUpgrades();
    renderReviews();
    updateDayClockUI();
    updateOrderUI();
  }

  /* =========================
     EVENT LISTENERS
  ========================= */

  document
    .querySelectorAll('.nav-btn')
    .forEach(btn => {
      btn.addEventListener(
        'click',
        () =>
          switchTab(
            btn.dataset.tab
          )
      );
    });

  document
    .querySelectorAll('.filter-btn')
    .forEach(btn => {
      btn.addEventListener(
        'click',
        () => {
          reviewFilter =
            btn.dataset.filter;

          renderReviews();
        }
      );
    });

  $('openCloseBtn')
    .addEventListener(
      'click',
      toggleShop
    );

  $('startBusinessBtn')
    .addEventListener(
      'click',
      () => {
        $('introModal')
          .classList
          .remove('visible');

        showToast(
          'Tiệm đã sẵn sàng. Bấm “MỞ CỬA TIỆM” nhé! 🍦'
        );
      }
    );

  $('serveBtn')
    .addEventListener(
      'click',
      deliverCustomer
    );

  $('resetScoopBtn')
    .addEventListener(
      'click',
      resetScoop
    );

  $('newDayBtn')
    .addEventListener(
      'click',
      newDay
    );

  /* =========================
     SAVE
  ========================= */

  window.addEventListener(
    'beforeunload',
    saveState
  );

  document.addEventListener(
    'visibilitychange',
    () => {
      if(document.hidden){
        saveState();
      }
    }
  );

  /* =========================
     STARTUP
  ========================= */

  state.shopTimer = null;

  updateTopStats();
  renderAll();

  /*
    Không cho game tự mở lại
    sau khi refresh.
  */
  if(state.isOpen){
    state.isOpen = false;
    state.daySeconds = 120;

    updateTopStats();
    updateDayClockUI();
  }
})();
