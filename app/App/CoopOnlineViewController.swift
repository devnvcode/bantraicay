import UIKit

/// GTA Online / Co-op Multiplayer Settings Page
/// Hỗ trợ chơi chung cả trong Story Mode (Cốt truyện) & Sandbox Mode
final class CoopOnlineViewController: UITableViewController {
    private var timer: Timer?

    init() {
        super.init(style: .insetGrouped)
        title = "GTA Online / Co-op Chơi Chung"
    }

    required init?(coder: NSCoder) { fatalError() }

    override func viewDidLoad() {
        super.viewDidLoad()
        Theme.styleSettingsTable(tableView)
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        timer = Timer.scheduledTimer(withTimeInterval: 0.6, repeats: true) { [weak self] _ in
            self?.tableView.reloadData()
        }
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        timer?.invalidate()
        timer = nil
    }

    override func numberOfSections(in tableView: UITableView) -> Int { 4 }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch section {
        case 0: return 3 // Bật/Tắt Co-op, Chế độ Host/Client, Trạng thái kết nối
        case 1: return 3 // Tên hiển thị, Địa chỉ IP/VPS, Cổng kết nối (Port)
        case 2: return 3 // Bypass phiên bản, Chấm xanh bản đồ, Đồng bộ xe & tọa độ 60 FPS
        case 3: return 2 // Kết nối lại, Đặt lại mặc định
        default: return 0
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        switch section {
        case 0: return "Trạng thái kết nối mạng"
        case 1: return "Thông tin người chơi & Máy chủ / VPS"
        case 2: return "Tính năng đồng bộ & Tương thích"
        case 3: return "Thao tác kết nối"
        default: return nil
        }
    }

    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        switch section {
        case 0:
            return "Chế độ Co-op Online hoạt động mượt mà ở cả Cốt truyện (Story Mode) và Tự do (Sandbox). Bật để kết nối vào thế giới của bạn bè."
        case 1:
            return "Nhập IP mạng LAN / WiFi, VPS hoặc Server riêng của bạn bè. Nếu chơi cùng qua điểm phát sóng di động (Hotspot/WiFi), một người làm Host và người kia nhập IP của Host."
        case 2:
            return "Hệ thống tự động vượt qua (bypass) kiểm tra phiên bản game, hiển thị chấm xanh (Blue Blip) cùng tên người chơi trên bản đồ thành phố, khóa cứng 60 FPS không giật lag."
        default: return nil
        }
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        if indexPath.section == 0 {
            let cell = UITableViewCell(style: .value1, reuseIdentifier: nil)

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Bật Chế độ Chơi Online / Co-op"
                cell.textLabel?.font = .systemFont(ofSize: 16, weight: .bold)
                let toggle = UISwitch()
                toggle.isOn = PGCoopIsActive()
                toggle.addAction(UIAction { [weak self] _ in
                    PGCoopSetActive(toggle.isOn)
                    self?.tableView.reloadData()
                }, for: .valueChanged)
                cell.accessoryView = toggle
                cell.selectionStyle = .none

            case 1:
                cell.textLabel?.text = "Chế độ phòng"
                cell.detailTextLabel?.textColor = Theme.secondaryText
                let count = PGCoopGetPlayerCount()
                cell.detailTextLabel?.text = count > 0 ? "Đang kết nối (\(count) người)" : "Client kết nối IP / VPS"
                cell.accessoryType = .disclosureIndicator

            case 2:
                let cell = UITableViewCell(style: .subtitle, reuseIdentifier: nil)
                cell.textLabel?.font = .systemFont(ofSize: 15, weight: .semibold)
                cell.textLabel?.text = "Tình trạng đường truyền"
                cell.detailTextLabel?.textColor = PGCoopIsActive() ? UIColor.systemGreen : UIColor.systemGray
                cell.detailTextLabel?.numberOfLines = 2

                var buf = [CChar](repeating: 0, count: 256)
                PGCoopGetStatus(&buf, buf.count)
                cell.detailTextLabel?.text = String(cString: buf)
                cell.selectionStyle = .none
                return cell

            default: break
            }
            return cell
        }

        if indexPath.section == 1 {
            let cell = UITableViewCell(style: .value1, reuseIdentifier: nil)
            cell.accessoryType = .disclosureIndicator

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Tên người chơi ngoài sảnh"
                var buf = [CChar](repeating: 0, count: 64)
                PGCoopGetPlayerName(&buf, buf.count)
                let name = String(cString: buf)
                cell.detailTextLabel?.text = name.isEmpty ? "Player" : name
                cell.detailTextLabel?.textColor = UIColor.systemBlue

            case 1:
                cell.textLabel?.text = "Địa chỉ IP / VPS Server"
                var buf = [CChar](repeating: 0, count: 64)
                PGCoopGetServerIP(&buf, buf.count)
                let ip = String(cString: buf)
                cell.detailTextLabel?.text = ip.isEmpty ? "127.0.0.1" : ip
                cell.detailTextLabel?.textColor = UIColor.systemGreen

            case 2:
                cell.textLabel?.text = "Cổng kết nối (Port)"
                let port = PGCoopGetServerPort()
                cell.detailTextLabel?.text = "\(port)"
                cell.detailTextLabel?.textColor = Theme.secondaryText

            default: break
            }
            return cell
        }

        if indexPath.section == 2 {
            let cell = UITableViewCell(style: .value1, reuseIdentifier: nil)
            cell.selectionStyle = .none

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Bỏ qua kiểm tra phiên bản (Bypass)"
                cell.detailTextLabel?.text = "Tự động tương thích"
                cell.detailTextLabel?.textColor = UIColor.systemGreen

            case 1:
                cell.textLabel?.text = "Dấu chấm xanh & Tên người chơi"
                cell.detailTextLabel?.text = "Toàn bản đồ"
                cell.detailTextLabel?.textColor = UIColor.systemBlue

            case 2:
                cell.textLabel?.text = "Đồng bộ xe, di chuyển & FPS"
                cell.detailTextLabel?.text = "Khóa cứng 60 FPS"
                cell.detailTextLabel?.textColor = Theme.accent

            default: break
            }
            return cell
        }

        if indexPath.section == 3 {
            let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
            cell.textLabel?.textAlignment = .center
            cell.textLabel?.font = .systemFont(ofSize: 16, weight: .semibold)

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Lưu thiết lập & Kết nối lại"
                cell.textLabel?.textColor = Theme.accent
            case 1:
                cell.textLabel?.text = "Đặt lại địa chỉ mặc định"
                cell.textLabel?.textColor = .systemRed
            default: break
            }
            return cell
        }

        return UITableViewCell()
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)

        if indexPath.section == 1 {
            switch indexPath.row {
            case 0:
                // Sửa tên người chơi
                var buf = [CChar](repeating: 0, count: 64)
                PGCoopGetPlayerName(&buf, buf.count)
                let current = String(cString: buf)

                let alert = UIAlertController(title: "Tên người chơi", message: "Nhập tên hiển thị trên bản đồ và trong phòng Co-op:", preferredStyle: .alert)
                alert.addTextField { tf in
                    tf.text = current.isEmpty ? "Player" : current
                    tf.placeholder = "Ví dụ: CuongGTA, Tommy, Franklin..."
                }
                alert.addAction(UIAlertAction(title: "Hủy", style: .cancel))
                alert.addAction(UIAlertAction(title: "Lưu", style: .default) { [weak self] _ in
                    let text = alert.textFields?.first?.text?.trimmingCharacters(in: .whitespacesAndNewlines) ?? "Player"
                    text.withCString { PGCoopSetPlayerName($0) }
                    self?.tableView.reloadData()
                })
                present(alert, animated: true)

            case 1:
                // Sửa địa chỉ IP / VPS
                var buf = [CChar](repeating: 0, count: 64)
                PGCoopGetServerIP(&buf, buf.count)
                let current = String(cString: buf)

                let alert = UIAlertController(title: "Địa chỉ IP / VPS Server", message: "Nhập địa chỉ IP mạng nội bộ, VPS hoặc IP server của bạn bè:", preferredStyle: .alert)
                alert.addTextField { tf in
                    tf.text = current.isEmpty ? "127.0.0.1" : current
                    tf.placeholder = "Ví dụ: 192.168.1.15, 103.x.x.x..."
                }
                alert.addAction(UIAlertAction(title: "Hủy", style: .cancel))
                alert.addAction(UIAlertAction(title: "Kết nối", style: .default) { [weak self] _ in
                    let text = alert.textFields?.first?.text?.trimmingCharacters(in: .whitespacesAndNewlines) ?? "127.0.0.1"
                    let port = PGCoopGetServerPort()
                    text.withCString { PGCoopSetServer($0, port) }
                    self?.tableView.reloadData()
                })
                present(alert, animated: true)

            case 2:
                // Sửa cổng kết nối (Port)
                let currentPort = PGCoopGetServerPort()
                let alert = UIAlertController(title: "Cổng kết nối (Port)", message: "Nhập cổng kết nối UDP (mặc định 4499):", preferredStyle: .alert)
                alert.addTextField { tf in
                    tf.text = "\(currentPort)"
                    tf.keyboardType = .numberPad
                }
                alert.addAction(UIAlertAction(title: "Hủy", style: .cancel))
                alert.addAction(UIAlertAction(title: "Lưu", style: .default) { [weak self] _ in
                    if let text = alert.textFields?.first?.text, let p = Int32(text), p > 0 && p < 65536 {
                        var ipBuf = [CChar](repeating: 0, count: 64)
                        PGCoopGetServerIP(&ipBuf, ipBuf.count)
                        PGCoopSetServer(&ipBuf, p)
                    }
                    self?.tableView.reloadData()
                })
                present(alert, animated: true)

            default: break
            }
        } else if indexPath.section == 3 {
            switch indexPath.row {
            case 0:
                // Kết nối lại
                var ipBuf = [CChar](repeating: 0, count: 64)
                PGCoopGetServerIP(&ipBuf, ipBuf.count)
                let port = PGCoopGetServerPort()
                PGCoopSetServer(&ipBuf, port)
                PGCoopSetActive(true)
                tableView.reloadData()

            case 1:
                // Đặt lại mặc định
                "127.0.0.1".withCString { PGCoopSetServer($0, 4499) }
                "Player".withCString { PGCoopSetPlayerName($0) }
                PGCoopSetHostMode(false)
                tableView.reloadData()

            default: break
            }
        }
    }
}
