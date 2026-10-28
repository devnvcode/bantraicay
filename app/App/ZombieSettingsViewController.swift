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
        case 0: return 1
        case 1: return 5
        case 2: return 1
        default: return 0
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        switch section {
        case 0: return "Trạng thái chế độ"
        case 1: return "Nhiệm vụ: Thoát khỏi thành phố"
        case 2: return "Thao tác"
        default: return nil
        }
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        if indexPath.section == 0 {
            let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
            cell.textLabel?.text = "Bật Chế độ Zombie Sinh tồn"
            cell.textLabel?.font = .systemFont(ofSize: 16, weight: .semibold)
            let toggle = UISwitch()
            toggle.isOn = PGZombieIsActive()
            toggle.addAction(UIAction { action in
                guard let sw = action.sender as? UISwitch else { return }
                PGZombieSetActive(sw.isOn)
            }, for: .valueChanged)
            cell.accessoryView = toggle
            cell.selectionStyle = .none
            return cell
        }

        if indexPath.section == 1 {
            let cell = UITableViewCell(style: .value1, reuseIdentifier: nil)
            cell.selectionStyle = .none
            let kills = PGZombieGetKills()
            let tier = PGZombieGetTier()
            let dist = PGZombieGetEscapeDistance()
            let escaped = PGZombieIsEscaped()

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Điểm sơ tán (Trực thăng Del Perro)"
                if escaped {
                    cell.detailTextLabel?.text = "ĐÃ THOÁT HIỂM THÀNH CÔNG!"
                    cell.detailTextLabel?.textColor = .systemGreen
                    cell.detailTextLabel?.font = .boldSystemFont(ofSize: 14)
                } else if dist < 45.0 {
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
                cell.textLabel?.text = "Người bạn đồng hành"
                let alive = PGZombieCompanionAlive()
                cell.detailTextLabel?.text = alive ? "Đang chiến đấu cùng bạn" : "Đang chờ hồi sinh/tiếp viện..."
                cell.detailTextLabel?.textColor = alive ? .systemGreen : .systemOrange
            case 4:
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
        cell.textLabel?.text = "Bắt đầu lại ván sinh tồn (Reset vũ khí & vị trí)"
        cell.textLabel?.textColor = .systemRed
        cell.textLabel?.textAlignment = .center
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
        }
    }

    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        if section == 0 {
            return "Thế giới hoang tàn giữa ban ngày (12h trưa), bầu trời u ám không còn sương mù dày đặc. Toàn bộ xe cộ trên đường đều tắt máy bỏ hoang, người dân biến thành xác sống hung hãn."
        }
        if section == 2 {
            return "HƯỚNG DẪN CHIẾN DỊCH:\n• Tất cả zombie trên bản đồ đều hiển thị chấm đỏ (kể cả ở xa).\n• Điểm sơ tán thoát khỏi thành phố nằm ở cuối Bến tàu Del Perro (Biểu tượng Trực thăng màu xanh lá trên bản đồ).\n• Chiến đấu mở khóa súng qua từng mốc kill và mở đường đến điểm sơ tán. Khi đến nơi, hãy cố thủ 45 giây trước làn sóng zombie dữ dội để trực thăng cứu hộ giải cứu thành công!"
        }
        return nil
    }
}
