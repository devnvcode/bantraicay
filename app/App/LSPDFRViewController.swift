import UIKit

/// LSPDFR Police Mod Menu & Mobile Data Terminal (MDT) for GTA V iOS Native Port
/// Tương thích 100% các mod từ https://www.lcpdfr.com
final class LSPDFRViewController: UITableViewController {
    private var timer: Timer?

    private enum Section: Int, CaseIterable {
        case status, agency, vehicles, callouts, backup, fieldActions, lcpdfrInfo
        var title: String {
            switch self {
            case .status: return "TRẠNG THÁI CA TRỰC (DUTY STATUS)"
            case .agency: return "ĐƠN VỊ & SẮC PHỤC NGÀNH"
            case .vehicles: return "ĐIỀU ĐỘNG ĐỘI XE CẢNH SÁT LCPDFR"
            case .callouts: return "TIẾP NHẬN TIN BÁO KHẨN CẤP (CALLOUTS)"
            case .backup: return "YÊU CẦU LỰC LƯỢNG VIỆN TRỢ (BACKUP)"
            case .fieldActions: return "NGHIỆP VỤ HIỆN TRƯỜNG & BẮT GIỮ"
            case .lcpdfrInfo: return "HƯỚNG DẪN CÀI MOD TỪ LCPDFR.COM"
            }
        }
    }

    private let agencies = [
        ("LSPD - Cảnh sát Thành phố", "Sắc phục sĩ quan tuần tra đô thị", 0),
        ("LSSD - Cảnh sát trưởng Sheriff", "Sắc phục hạt Los Santos & ngoại ô", 1),
        ("SAHP - Tuần tra Cao tốc", "Đội tuần tra đường dài & cao tốc", 2),
        ("SWAT / NOOSE - Đội Đặc nhiệm", "Trang bị giáp nặng & vũ khí chiến thuật", 3),
        ("FIB - Cục Điều tra Liên bang", "Đặc vụ ngầm đồ vest chuyên án", 4)
    ]

    private let vehicles = [
        ("Stanier LSPD Cruiser", "police", 0),
        ("Buffalo Interceptor", "police2", 1),
        ("Vapid Interceptor", "police3", 2),
        ("Unmarked Interceptor (Xe ngầm)", "police4", 3),
        ("Sheriff Cruiser", "sheriff", 4),
        ("Sheriff Granger SUV", "sheriff2", 5),
        ("Prison Transport (Xe áp giải)", "policet", 6),
        ("SWAT Armored Riot (Xe bọc thép)", "riot", 7),
        ("FIB Buffalo", "fbi", 8),
        ("FIB Granger SUV", "fbi2", 9),
        ("Police Motorcycle (Mô tô tuần tra)", "policeb", 10),
        ("Police Maverick (Trực thăng)", "polmav", 11)
    ]

    private let callouts = [
        ("10-99: Đuổi bắt xe vượt tốc độ", "Nghi phạm lái xe bỏ chạy lạng lách nguy hiểm", 1),
        ("211: Cướp có vũ trang", "Nghi phạm nổ súng cướp tiệm, cần can thiệp", 2),
        ("245: Đấu súng nguy cấp", "Băng đảng thanh trừng nổ súng tại khu dân cư", 3),
        ("10-50: Xe cảnh sát bị cướp đào tẩu", "Đối tượng cướp xe tuần tra đang chạy trốn", 4)
    ]

    private let backups = [
        ("Viện trợ Đội tuần tra địa phương (Code 2/3)", "1-2 xe tuần tra lao tới hiện trường", 1),
        ("Điều động Đội Đặc nhiệm SWAT / NOOSE", "Xe bọc thép chở 4 đặc nhiệm súng trường", 2),
        ("Không quân / Trực thăng tuần tra (Air Support)", "Trực thăng rọi đèn theo dấu nghi phạm", 3),
        ("Xe Cứu thương cấp cứu EMS (Paramedics)", "Nhân viên y tế sơ cứu người bị thương", 4)
    ]

    private let actions = [
        ("Dừng xe vi phạm (Traffic Stop)", "Tấp xe phía trước vào lề, tắt máy chờ kiểm tra"),
        ("Bắt giữ / Còng tay nghi phạm", "Nghi phạm giơ tay đầu hàng hoặc quỳ xuống"),
        ("Điều xe áp giải phạm nhân về đồn", "Xe vận chuyển tới đón nghi phạm đã bắt"),
        ("Bật / Tắt Radar đo tốc độ xe (ALPR)", "Quét tốc độ và biển số xe trước mặt")
    ]

    init() {
        super.init(style: .insetGrouped)
        title = "LSPDFR - Cảnh sát Phản ứng nhanh"
        modalPresentationStyle = .formSheet
        overrideUserInterfaceStyle = .dark
    }

    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    override func viewDidLoad() {
        super.viewDidLoad()
        navigationItem.rightBarButtonItem = UIBarButtonItem(title: "Đóng", primaryAction: UIAction { [weak self] _ in
            self?.dismiss(animated: true)
        })
        navigationItem.rightBarButtonItem?.tintColor = Theme.accent
        Theme.styleSettingsTable(tableView)
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.tableView.reloadSections(IndexSet(integer: Section.status.rawValue), with: .none)
        }
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        timer?.invalidate()
        timer = nil
    }

    override func numberOfSections(in tableView: UITableView) -> Int { Section.allCases.count }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        guard let s = Section(rawValue: section) else { return 0 }
        switch s {
        case .status: return 2
        case .agency: return agencies.count
        case .vehicles: return vehicles.count
        case .callouts: return callouts.count
        case .backup: return backups.count
        case .fieldActions: return actions.count
        case .lcpdfrInfo: return 1
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        Section(rawValue: section)?.title
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let s = Section(rawValue: indexPath.section) else { return UITableViewCell() }
        let cell = UITableViewCell(style: .subtitle, reuseIdentifier: nil)
        cell.backgroundColor = Theme.rowBackground
        cell.textLabel?.textColor = Theme.text
        cell.detailTextLabel?.textColor = Theme.secondaryText

        switch s {
        case .status:
            if indexPath.row == 0 {
                cell.textLabel?.text = "Vào ca trực (Go On Duty)"
                cell.detailTextLabel?.text = PGLSPDFRIsOnDuty() ? "Đang trong ca trực: Được trang bị súng cảnh sát, áo giáp, miễn trừ truy nã." : "Đang ngoài ca trực: Người chơi tự do."
                let toggle = UISwitch()
                toggle.isOn = PGLSPDFRIsOnDuty()
                toggle.onTintColor = Theme.accent
                toggle.addAction(UIAction { _ in
                    let newState = toggle.isOn
                    PGLSPDFRSetDuty(newState)
                    cell.detailTextLabel?.text = newState ? "Đang trong ca trực: Được trang bị súng cảnh sát, áo giáp, miễn trừ truy nã." : "Đang ngoài ca trực: Người chơi tự do."
                }, for: .valueChanged)
                cell.accessoryView = toggle
            } else {
                var buf = [CChar](<repeating: 0, count: 128>)
                PGLSPDFRGetStatus(&buf, 128)
                let statusStr = String(cString: buf)
                cell.textLabel?.text = "Bản tin bộ đàm / Radar"
                cell.detailTextLabel?.text = statusStr.isEmpty ? "Hệ thống sẵn sàng" : statusStr
                cell.detailTextLabel?.textColor = Theme.accent
            }
        case .agency:
            let item = agencies[indexPath.row]
            cell.textLabel?.text = item.0
            cell.detailTextLabel?.text = item.1
            cell.accessoryType = (PGLSPDFRGetAgency() == item.2) ? .checkmark : .none
        case .vehicles:
            let item = vehicles[indexPath.row]
            cell.textLabel?.text = item.0
            cell.detailTextLabel?.text = "Model: \(item.1) (Hỗ trợ mod xe thay thế & Addon)"
            cell.accessoryType = .disclosureIndicator
        case .callouts:
            let item = callouts[indexPath.row]
            cell.textLabel?.text = item.0
            cell.detailTextLabel?.text = item.1
            cell.accessoryType = .disclosureIndicator
        case .backup:
            let item = backups[indexPath.row]
            cell.textLabel?.text = item.0
            cell.detailTextLabel?.text = item.1
            cell.accessoryType = .disclosureIndicator
        case .fieldActions:
            let item = actions[indexPath.row]
            cell.textLabel?.text = item.0
            cell.detailTextLabel?.text = item.1
            cell.accessoryType = (indexPath.row == 3) ? (PGLSPDFRIsRadarActive() ? .checkmark : .none) : .disclosureIndicator
        case .lcpdfrInfo:
            cell.textLabel?.text = "Cách cài Mod từ LCPDFR.com"
            cell.detailTextLabel?.text = "Tải file .oiv, zip xe cảnh sát, âm thanh còi hú -> Bỏ vào thư mục Mods trên máy -> Hệ thống tự động nhận diện 100%!"
            cell.detailTextLabel?.numberOfLines = 0
            cell.accessoryType = .none
        }
        return cell
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        guard let s = Section(rawValue: indexPath.section) else { return }
        switch s {
        case .status: break
        case .agency:
            PGLSPDFRSetAgency(Int32(agencies[indexPath.row].2))
            if !PGLSPDFRIsOnDuty() { PGLSPDFRSetDuty(true) }
            tableView.reloadSections(IndexSet(integer: Section.agency.rawValue), with: .none)
            dismiss(animated: true)
        case .vehicles:
            if !PGLSPDFRIsOnDuty() { PGLSPDFRSetDuty(true) }
            PGLSPDFRSpawnVehicle(Int32(vehicles[indexPath.row].2))
            dismiss(animated: true)
        case .callouts:
            if !PGLSPDFRIsOnDuty() { PGLSPDFRSetDuty(true) }
            PGLSPDFRTriggerCallout(Int32(callouts[indexPath.row].2))
            dismiss(animated: true)
        case .backup:
            if !PGLSPDFRIsOnDuty() { PGLSPDFRSetDuty(true) }
            PGLSPDFRRequestBackup(Int32(backups[indexPath.row].2))
            dismiss(animated: true)
        case .fieldActions:
            switch indexPath.row {
            case 0: PGLSPDFRTrafficStop()
            case 1: PGLSPDFRArrestNearby()
            case 2: PGLSPDFRCallTransport()
            case 3:
                PGLSPDFRToggleRadar()
                tableView.reloadRows(at: [indexPath], with: .none)
                return
            default: break
            }
            dismiss(animated: true)
        case .lcpdfrInfo: break
        }
    }
}
