/* =========================================================
   TIỆM KEM NGỌT NGÀO
   DATA ONLY
========================================================= */

const ICE_CREAM_FLAVORS = [

  {
    id: "choco",
    name: "Sô-cô-la",
    emoji: "🍫",
    color: "#79503e",
    price: 24,
    packCost: 80
  },

  {
    id: "strawberry",
    name: "Dâu Tây",
    emoji: "🍓",
    color: "#ee9caf",
    price: 22,
    packCost: 78
  },

  {
    id: "vanilla",
    name: "Vani",
    emoji: "🍦",
    color: "#fff0b5",
    price: 20,
    packCost: 74
  },

  {
    id: "matcha",
    name: "Matcha",
    emoji: "🍵",
    color: "#a9d39b",
    price: 24,
    packCost: 82
  },

  {
    id: "avocado",
    name: "Bơ",
    emoji: "🥑",
    color: "#b7cc75",
    price: 23,
    packCost: 78
  },

  {
    id: "durian",
    name: "Sầu Riêng",
    emoji: "🥭",
    color: "#f1dc7c",
    price: 26,
    packCost: 86
  },

  {
    id: "taro",
    name: "Khoai Môn",
    emoji: "🍠",
    color: "#c7a9df",
    price: 23,
    packCost: 78
  },

  {
    id: "oreo",
    name: "Oreo",
    emoji: "🍪",
    color: "#85858b",
    price: 25,
    packCost: 84
  },

  {
    id: "yogurt",
    name: "Sữa Chua",
    emoji: "🥛",
    color: "#dbe7f0",
    price: 21,
    packCost: 76
  },

  {
    id: "rainbow",
    name: "Rainbow",
    emoji: "🌈",
    color: "#f2bfd7",
    price: 28,
    packCost: 92
  }

];


const TOPPINGS = [

  {
    id: "rainbow",
    name: "Cốm Rainbow",
    emoji: "🌈",
    price: 4
  },

  {
    id: "chocoSauce",
    name: "Sốt Chocolate",
    emoji: "🍫",
    price: 5
  },

  {
    id: "nuts",
    name: "Hạt Dẻ",
    emoji: "🌰",
    price: 5
  },

  {
    id: "freshStrawberry",
    name: "Dâu Tươi",
    emoji: "🍓",
    price: 6
  },

  {
    id: "oreoCrumb",
    name: "Vụn Oreo",
    emoji: "🍪",
    price: 5
  },

  {
    id: "marshmallow",
    name: "Marshmallow",
    emoji: "🤍",
    price: 4
  },

  {
    id: "wafer",
    name: "Bánh Wafer",
    emoji: "🍫",
    price: 4
  },

  {
    id: "cherry",
    name: "Cherry",
    emoji: "🍒",
    price: 5
  }

];


const CUSTOMER_NAMES = [

  "Hoàng Nam",
  "Thu Trang",
  "Bảo Anh",
  "Ngọc Linh",
  "Minh Đức",
  "Mai Phương",
  "Tuấn Kiệt",
  "Khánh An",
  "Gia Hân",
  "Quang Huy",
  "Thanh Tùng",
  "Hà My",
  "Đức Anh",
  "Phương Nhi",
  "Kim Ngân",
  "Anh Khoa",
  "Yến Nhi",
  "Minh Khang",
  "Lan Chi",
  "Trọng Nghĩa",
  "Nhật Minh",
  "Thảo Vy",
  "Hữu Phước",
  "Mỹ Duyên",
  "Quỳnh Anh",
  "Đình Phong",
  "Ngọc Hân",
  "Tấn Phát",
  "Hoài An",
  "Bích Ngọc",
  "Đăng Khoa",
  "Thiên Kim",
  "Mộc Miên",
  "Khôi Nguyên",
  "Tú Uyên",
  "An Nhiên",
  "Đình Quân",
  "Thanh Vy",
  "Khánh Duy",
  "Uyên Nhi",
  "Minh Anh",
  "Đức Minh",
  "Hạ Vy",
  "Thiên An",
  "Hoàng Phúc",
  "Linh Đan",
  "Gia Bảo",
  "Ngọc Mai"
];


const AVATARS = [
  "🧑🏻",
  "👩🏻",
  "🧑🏼",
  "👩🏼",
  "🧑🏽",
  "👨🏻",
  "👩🏽",
  "🧑🏾",
  "👨🏽",
  "👩🏾",
  "👧🏻",
  "👦🏻",
  "👨🏼",
  "👩🏻‍🦰"
];


const HAIR_TYPES = [
  "short",
  "long",
  "curly",
  "bob",
  "cap"
];


const SHIRT_COLORS = [
  "#f4a9b8",
  "#9ed2c0",
  "#f2cf7b",
  "#a8bde5",
  "#d5a9df",
  "#f2b38a"
];


const SKIN_TONES = [
  "#f4c7a1",
  "#e4ae7e",
  "#c88d62",
  "#f7d3b1"
];


const POSITIVE_REVIEWS = [

  "Kem sô-cô-la ở đây ngon xỉu, múc nhanh và rất thơm!",
  "Matcha béo nhẹ, ăn xong vẫn muốn gọi thêm một viên.",
  "Tiệm xinh xắn, giao kem nhanh và rất dễ thương.",
  "Kem dâu thơm, mềm mịn, vị ngọt vừa phải.",
  "Vani nhẹ nhàng, ăn một viên là muốn ăn thêm.",
  "Nhân viên thân thiện, quầy sạch sẽ và gọn gàng.",
  "Kem bơ béo vừa phải, không bị ngấy.",
  "Sầu riêng thơm nức, đúng kiểu mình thích.",
  "Khoai môn thơm dịu, màu kem nhìn rất xinh.",
  "Oreo giòn giòn, kem mát lạnh, cực hợp buổi chiều.",
  "Sữa chua chua nhẹ, ăn trời nóng rất đã.",
  "Rainbow nhiều màu nhìn siêu vui mắt.",
  "Giá hợp lý, phục vụ nhanh, mình rất hài lòng.",
  "Chủ tiệm nói chuyện dễ thương, kem cũng ngon.",
  "Đợi đúng một chút thôi nhưng ly kem rất đáng.",
  "Quầy kem nhỏ mà nhiều vị ghê, tha hồ chọn.",
  "Kem mịn, topping đẹp, trải nghiệm rất ổn.",
  "Mình thích nhất vị matcha, thơm mà không quá đắng.",
  "Dâu Tây thơm và có vị rất dễ ăn.",
  "Một góc phố thật dễ thương, lần sau mình sẽ quay lại.",
  "Kem vani dịu và thơm, đúng gu mình.",
  "Sầu riêng ngon bất ngờ, béo mà không quá nặng.",
  "Quầy bán hàng gọn gàng, thao tác rất nhanh.",
  "Đúng món, đúng vị, phục vụ dễ thương.",
  "Ly kem nhìn xinh hơn mình tưởng, ăn cũng ngon nữa.",
  "Topping dâu tươi rất hợp với kem vani.",
  "Chocolate thơm, vị đậm vừa đủ.",
  "Nhân viên làm kem nhanh mà vẫn rất cẩn thận.",
  "Mình thích cách tiệm trang trí, nhìn rất ấm áp.",
  "Kem mát lạnh, topping đầy đặn, rất đáng tiền.",
  "Đến cùng bạn mà cả hai đều thích tiệm.",
  "Ly kem nhỏ xinh, ăn xong tâm trạng cũng vui.",
  "Phục vụ đúng món mình gọi, không bị nhầm vị.",
  "Topping Rainbow nhìn cực kỳ bắt mắt.",
  "Kem khoai môn thơm dịu và không quá ngọt.",
  "Sữa chua là món mình sẽ gọi lại lần sau.",
  "Một tiệm kem nhỏ nhưng làm rất có tâm.",
  "Mình sẽ dẫn thêm bạn bè tới thử.",
  "Thao tác nhanh, khách không phải chờ lâu.",
  "Kem ngon và quầy rất sạch sẽ.",
  "Vừa ăn vừa chụp ảnh rất hợp luôn.",
  "Hôm nay có một trải nghiệm rất vui."
];


const NEGATIVE_REVIEWS = [

  "Múc kem kiểu gì lâu thế!",
  "Bán buôn chán quá, mình đi về đây.",
  "Đợi lâu quá nên mình hết kiên nhẫn.",
  "Chờ mãi vẫn chưa có kem.",
  "Mình đứng ở quầy lâu hơn dự kiến.",
  "Kem chưa tới mà mình đã muốn về.",
  "Hôm nay tốc độ phục vụ hơi chậm.",
  "Đợi một viên kem mà cảm giác rất lâu.",
  "Mình bỏ cuộc vì không thể chờ thêm.",
  "Tiệm dễ thương nhưng hôm nay phục vụ chậm.",
  "Đứng đợi mãi mà đơn chưa xong.",
  "Mình thích vị kem nhưng không thích phải chờ.",
  "Hôm nay trải nghiệm chưa được như mong đợi.",
  "Khách tới trước mà phải đợi khá lâu.",
  "Hy vọng lần sau tiệm phục vụ nhanh hơn.",
  "Đợi quá lâu nên chỉ có thể cho 1 sao.",
  "Không nghĩ một ly kem lại mất nhiều thời gian.",
  "Mong tiệm cải thiện tốc độ phục vụ.",
  "Mình đành đi về vì hết kiên nhẫn.",
  "Quầy đông nhưng xử lý đơn hơi chậm.",
  "Hôm nay chưa có trải nghiệm tốt.",
  "Chờ lâu làm mình mất hết tâm trạng ăn kem.",
  "Kem chưa xong mà mình phải rời đi.",
  "Hy vọng lần tới sẽ nhanh hơn nhé.",
  "Đứng chờ khá lâu nhưng chưa nhận được món.",
  "Mình mong tiệm xử lý đơn nhanh hơn.",
  "Chờ lâu quá nên không thể đợi tiếp.",
  "Hôm nay mình chưa hài lòng với tốc độ.",
  "Đơn nhỏ nhưng chờ hơi lâu.",
  "Mình phải về trước vì không còn thời gian.",
  "Tiệm cần tăng tốc lúc đông khách.",
  "Đợi tới lúc mất kiên nhẫn luôn.",
  "Mong lần sau không phải chờ lâu như vậy.",
  "Hôm nay kem ngon cũng không cứu được thời gian chờ.",
  "Mình chưa nhận được món trước khi phải đi.",
  "Chờ quá lâu nên trải nghiệm không vui.",
  "Tốc độ hôm nay hơi chậm.",
  "Hy vọng shop phục vụ nhanh hơn.",
  "Mình không thể đợi thêm nữa.",
  "Lần này mình phải cho 1 sao vì chờ quá lâu."
];


const CUSTOMER_REQUEST_TEMPLATES = {

  female: [
    "Dạ shop múc cho em {order} với ạ!",
    "Cho chị {order} nha shop!",
    "Shop ơi em lấy {order} nhé!",
    "Em muốn {order} ạ!",
    "Cho em một phần {order} xinh xinh nha!"
  ],

  male: [
    "Anh lấy {order} mát lạnh nhé!",
    "Shop cho anh {order} với!",
    "Cho mình {order} nha shop!",
    "Mình lấy {order} nhé!",
    "Cho anh một phần {order} nha!"
  ]

};


const TIME_LABELS = [
  "vừa xong",
  "1 phút trước",
  "3 phút trước",
  "5 phút trước",
  "8 phút trước",
  "12 phút trước",
  "15 phút trước"
];
