import UIKit

final class ZombieSettingsViewController: UITableViewController {
    private var timer: Timer?

    init() {
        super.init(style: .insetGrouped)
        title = "Đại dịch Zombie Sinh tồn"
    }

    required init?(coder: NSCoder) { fatalError() }

    override func viewDidLoad() {
        super.viewDidLoad()
        Theme.styleSettingsTable(tableView)
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        timer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] _ in
            self?.tableView.reloadData()
        }
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        timer?.invalidate()
        timer = nil
    }

    override func numberOfSections(in tableView: UITableView) -> Int { 3 }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch section {
        case 0: return 5 // Master toggle + 4 submodule toggles
        case 1: return 7 // Stats: Evac, Kills, Weapon Tier, Active Zombies, Wrecks, Companion, Status
        case 2: return 2 // Reset button + View Log button
        default: return 0
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        switch section {
        case 0: return "Cấu hình module"
        case 1: return "Thống kê sinh tồn & Nhiệm vụ"
        case 2: return "Thao tác & Nhật ký"
        default: return nil
        }
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        if indexPath.section == 0 {
            let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
            cell.selectionStyle = .none
            let toggle = UISwitch()

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Bật Chế độ Zombie Sinh tồn (Chính)"
                cell.textLabel?.font = .systemFont(ofSize: 16, weight: .bold)
                toggle.isOn = PGZombieIsActive()
                toggle.addAction(UIAction { action in
                    guard let sw = action.sender as? UISwitch else { return }
                    PGZombieSetActive(sw.isOn)
                }, for: .valueChanged)
            case 1:
                cell.textLabel?.text = "Bầu không khí & Thời tiết u ám"
                cell.textLabel?.font = .systemFont(ofSize: 15)
                toggle.isOn = PGZombieGetModuleAtmosphere()
                toggle.addAction(UIAction { action in
                    guard let sw = action.sender as? UISwitch else { return }
                    PGZombieSetModuleAtmosphere(sw.isOn)
                }, for: .valueChanged)
            case 2:
                cell.textLabel?.text = "Xe cộ bỏ hoang trên đường"
                cell.textLabel?.font = .systemFont(ofSize: 15)
                toggle.isOn = PGZombieGetModuleWrecks()
                toggle.addAction(UIAction { action in
                    guard let sw = action.sender as? UISwitch else { return }
                    PGZombieSetModuleWrecks(sw.isOn)
                }, for: .valueChanged)
            case 3:
                cell.textLabel?.text = "Đồng đội sinh tồn đi cùng"
                cell.textLabel?.font = .systemFont(ofSize: 15)
                toggle.isOn = PGZombieGetModuleCompanion()
                toggle.addAction(UIAction { action in
                    guard let sw = action.sender as? UISwitch else { return }
                    PGZombieSetModuleCompanion(sw.isOn)
                }, for: .valueChanged)
            case 4:
                cell.textLabel?.text = "Nhiệm vụ Sơ tán Del Perro Pier"
                cell.textLabel?.font = .systemFont(ofSize: 15)
                toggle.isOn = PGZombieGetModuleExtraction()
                toggle.addAction(UIAction { action in
                    guard let sw = action.sender as? UISwitch else { return }
                    PGZombieSetModuleExtraction(sw.isOn)
                }, for: .valueChanged)
            default: break
            }
            cell.accessoryView = toggle
            return cell
        }

        if indexPath.section == 1 {
            let cell = UITableViewCell(style: .value1, reuseIdentifier: nil)
            cell.selectionStyle = .none
            let kills = PGZombieGetKills()
            let tier = PGZombieGetTier()
            let dist = PGZombieGetEscapeDistance()
            let escaped = PGZombieIsEscaped()
            let activeZ = PGZombieGetActiveCount()
            let activeW = PGZombieGetWreckCount()

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Điểm sơ tán (Trực thăng Del Perro)"
                if escaped {
                    cell.detailTextLabel?.text = "ĐÃ THOÁT HIỂM THÀNH CÔNG!"
                    cell.detailTextLabel?.textColor = .systemGreen
                    cell.detailTextLabel?.font = .boldSystemFont(ofSize: 14)
                } else if dist < 40.0 {
                    cell.detailTextLabel?.text = "Đang trong Vùng Cố thủ!"
                    cell.detailTextLabel?.textColor = .systemRed
                    cell.detailTextLabel?.font = .boldSystemFont(ofSize: 14)
                } else {
                    cell.detailTextLabel?.text = String(format: "%.0f mét (Chấm xanh lá)", dist)
                    cell.detailTextLabel?.textColor = Theme.accent
                    cell.detailTextLabel?.font = .boldSystemFont(ofSize: 14)
                }
            case 1:
                cell.textLabel?.text = "Zombie đã tiêu diệt"
                cell.detailTextLabel?.text = "\(kills) con"
                cell.detailTextLabel?.textColor = Theme.accent
                cell.detailTextLabel?.font = .boldSystemFont(ofSize: 16)
            case 2:
                cell.textLabel?.text = "Cấp độ trang bị vũ khí"
                let names = [
                    "Cấp 0 (Cận chiến)",
                    "Cấp 1 (Súng lục - 5 kills)",
                    "Cấp 2 (Shotgun - 15 kills)",
                    "Cấp 3 (Micro SMG - 30 kills)",
                    "Cấp 4 (Carbine Rifle - 50 kills)",
                    "Cấp 5 (Combat MG & Shotgun - 75 kills)"
                ]
                cell.detailTextLabel?.text = (tier >= 0 && Int(tier) < names.count) ? names[Int(tier)] : "Cấp \(tier)"
                cell.detailTextLabel?.textColor = .label
            case 3:
                cell.textLabel?.text = "Zombie đang hoạt động"
                cell.detailTextLabel?.text = "\(activeZ)/12 con"
                cell.detailTextLabel?.textColor = .label
            case 4:
                cell.textLabel?.text = "Xe bỏ hoang xung quanh"
                cell.detailTextLabel?.text = "\(activeW)/5 xe"
                cell.detailTextLabel?.textColor = .label
            case 5:
                cell.textLabel?.text = "Người bạn đồng hành"
                let alive = PGZombieCompanionAlive()
                cell.detailTextLabel?.text = alive ? "Đang chiến đấu cùng bạn" : "Đang chờ hồi sinh/tiếp viện..."
                cell.detailTextLabel?.textColor = alive ? .systemGreen : .systemOrange
            case 6:
                cell.textLabel?.text = "Tình hình chiến dịch"
                var buf = [CChar](repeating: 0, count: 160)
                PGZombieGetStatus(&buf, buf.count)
                let statusStr = String(cString: buf)
                cell.detailTextLabel?.text = statusStr.isEmpty ? "Đang tiến hành" : statusStr
                cell.detailTextLabel?.font = .systemFont(ofSize: 12)
            default: break
            }
            return cell
        }

        let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
        if indexPath.row == 0 {
            cell.textLabel?.text = "Bắt đầu lại ván sinh tồn (Reset vũ khí & vị trí)"
            cell.textLabel?.textColor = .systemRed
            cell.textLabel?.textAlignment = .center
        } else {
            cell.textLabel?.text = "Xem nhật ký sinh tồn (zombie_survival.log)"
            cell.textLabel?.textColor = Theme.accent
            cell.textLabel?.textAlignment = .center
        }
        return cell
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        if indexPath.section == 2 && indexPath.row == 0 {
            let alert = UIAlertController(title: "Bắt đầu lại", message: "Khởi động lại toàn bộ chiến dịch sinh tồn và tước súng về cận chiến?", preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: "Hủy", style: .cancel))
            alert.addAction(UIAlertAction(title: "Đồng ý", style: .destructive, handler: { _ in
                PGZombieResetSurvival()
            }))
            present(alert, animated: true)
        } else if indexPath.section == 2 && indexPath.row == 1 {
            showZombieLogViewer()
        }
    }

    private func showZombieLogViewer() {
        var memLog = ""
        var buf = [CChar](repeating: 0, count: 4096)
        PGZombieGetLog(&buf, buf.count)
        if buf[0] != 0 {
            memLog = String(cString: buf)
        }

        let logURL = ModInstaller.modsFolder.appendingPathComponent("zombie_survival.log")
        let fileLog = (try? String(contentsOf: logURL, encoding: .utf8)) ?? ""

        let content: String
        if !memLog.isEmpty {
            content = memLog + (fileLog.isEmpty ? "" : "\n--- File Log ---\n" + fileLog)
        } else if !fileLog.isEmpty {
            content = fileLog
        } else {
            content = "(Chưa có dữ liệu nhật ký hoặc file chưa được tạo)"
        }

        let textVC = UIViewController()
        textVC.title = "Nhật ký Zombie Survival"
        let textView = UITextView(frame: textVC.view.bounds)
        textView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        textView.isEditable = false
        textView.font = .monospacedSystemFont(ofSize: 12, weight: .regular)
        textView.text = content
        textVC.view.addSubview(textView)

        let nav = UINavigationController(rootViewController: textVC)
        textVC.navigationItem.rightBarButtonItem = UIBarButtonItem(systemItem: .done, primaryAction: UIAction { _ in
            nav.dismiss(animated: true)
        })
        present(nav, animated: true)
    }

    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        if section == 0 {
            return "Khởi tạo tuần tự và trì hoãn sau khi map tải xong giúp iPhone chạy mượt mà, không giật lag và không bị sập game."
        }
        if section == 2 {
            return "HƯỚNG DẪN CHIẾN DỊCH:\n• Tất cả zombie trên bản đồ đều hiển thị chấm đỏ từ xa.\n• Điểm sơ tán thoát khỏi thành phố nằm ở cuối Bến tàu Del Perro (Biểu tượng Trực thăng màu xanh lá).\n• Hãy chiến đấu bằng vũ khí cận chiến để mở khóa súng lục, shotgun, súng trường và tiến tới bến tàu cố thủ 45 giây để trốn thoát thành công!"
        }
        return nil
    }
}
