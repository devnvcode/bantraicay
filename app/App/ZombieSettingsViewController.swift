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
        case 1: return 4
        case 2: return 1
        default: return 0
        }
    }

    override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        switch section {
        case 0: return "Trạng thái chế độ"
        case 1: return "Thông số chiến dịch sinh tồn"
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

            switch indexPath.row {
            case 0:
                cell.textLabel?.text = "Zombie đã tiêu diệt"
                cell.detailTextLabel?.text = "\(kills) con"
                cell.detailTextLabel?.textColor = Theme.accent
                cell.detailTextLabel?.font = .boldSystemFont(ofSize: 16)
            case 1:
                cell.textLabel?.text = "Cấp độ trang bị vũ khí"
                let names = [
                    "Cấp 0 (Chỉ Cận chiến)",
                    "Cấp 1 (Súng lục Combat Pistol)",
                    "Cấp 2 (Pump Shotgun)",
                    "Cấp 3 (Micro SMG)",
                    "Cấp 4 (Carbine Rifle)",
                    "Cấp 5 (Combat MG & Shotgun)"
                ]
                cell.detailTextLabel?.text = (tier >= 0 && Int(tier) < names.count) ? names[Int(tier)] : "Cấp \(tier)"
                cell.detailTextLabel?.textColor = .label
            case 2:
                cell.textLabel?.text = "Người bạn đồng hành"
                let alive = PGZombieCompanionAlive()
                cell.detailTextLabel?.text = alive ? "Đang yểm trợ chiến đấu" : "Đang chờ hồi sinh/tiếp viện..."
                cell.detailTextLabel?.textColor = alive ? .systemGreen : .systemOrange
            case 3:
                cell.textLabel?.text = "Trạng thái tình huống"
                var buf = [CChar](repeating: 0, count: 160)
                PGZombieGetStatus(&buf, buf.count)
                let statusStr = String(cString: buf)
                cell.detailTextLabel?.text = statusStr.isEmpty ? "Bình thường" : statusStr
                cell.detailTextLabel?.font = .systemFont(ofSize: 13)
            default: break
            }
            return cell
        }

        let cell = UITableViewCell(style: .default, reuseIdentifier: nil)
        cell.textLabel?.text = "Bắt đầu lại ván sinh tồn (Reset vũ khí & kill)"
        cell.textLabel?.textColor = .systemRed
        cell.textLabel?.textAlignment = .center
        return cell
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        if indexPath.section == 2 && indexPath.row == 0 {
            let alert = UIAlertController(title: "Bắt đầu lại", message: "Khởi động lại toàn bộ tiến trình sinh tồn, tước hết súng về chỉ dùng cận chiến?", preferredStyle: .alert)
            alert.addAction(UIAlertAction(title: "Hủy", style: .cancel))
            alert.addAction(UIAlertAction(title: "Đồng ý", style: .destructive, handler: { _ in
                PGZombieResetSurvival()
            }))
            present(alert, animated: true)
        }
    }

    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        if section == 0 {
            return "Thế giới hoang tàn, sương mù bao phủ ban ngày, xe cộ vứt bỏ ngổn ngang. Toàn bộ người đi bộ và cảnh sát biến mất, chỉ còn thây ma và người bạn đồng hành sống sót."
        }
        if section == 2 {
            return "HƯỚNG DẪN SINH TỒN:\n• Khởi đầu chỉ có vũ khí cận chiến (dao, mã tấu, xà beng). Tiêu diệt đủ mốc zombie để tự động nhận thêm súng và đạn dược.\n• Người bạn đồng hành được trang bị súng và sẽ tự động bắn zombie giải vây khi bị áp sát.\n• Zombie có chấm đỏ trên radar; đồng đội có chấm xanh dương."
        }
        return nil
    }
}
