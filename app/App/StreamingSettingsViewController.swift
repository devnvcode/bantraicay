import UIKit

final class StreamingSettingsViewController: UITableViewController {
    init() { super.init(style: .insetGrouped); title = "Khoảng cách và ngân sách texture" }
    required init?(coder: NSCoder) { fatalError() }
    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int { 2 }
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = UITableViewCell(style: .subtitle, reuseIdentifier: nil)
        if indexPath.row == 0 {
            cell.textLabel?.text = "Extended Distance Scaling"
            cell.detailTextLabel?.text = StreamingSettings.distanceScale.map { "Tầm nhìn mở rộng: \(Int((Double($0) ?? 0) * 100))%" } ?? "Tự động (giữ bản nền)"
        } else {
            cell.textLabel?.text = "Extended Texture Budget"
            cell.detailTextLabel?.text = StreamingSettings.textureBudgetMB == 0 ? "Tự động theo giới hạn app và GPU" : "Ngân sách texture: \(StreamingSettings.textureBudgetMB) MiB"
        }
        cell.accessoryType = .disclosureIndicator
        return cell
    }
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        let menu = UIAlertController(title: indexPath.row == 0 ? "Tầm nhìn mở rộng" : "Ngân sách texture", message: nil, preferredStyle: .alert)
        menu.addAction(UIAlertAction(title: "Tự động (bản nền)", style: .default) { _ in
            if indexPath.row == 0 { StreamingSettings.distanceScale = nil } else { StreamingSettings.textureBudgetMB = 0 }
            tableView.reloadData()
        })
        let values = indexPath.row == 0 ? StreamingSettings.distanceValues : StreamingSettings.textureValues.map(String.init)
        for value in values {
            let title = indexPath.row == 0 ? "\(Int((Double(value) ?? 0) * 100))%" : "\(value) MiB"
            menu.addAction(UIAlertAction(title: title, style: .default) { _ in
                if indexPath.row == 0 { StreamingSettings.distanceScale = value } else { StreamingSettings.textureBudgetMB = Int(value) ?? 0 }
                tableView.reloadData()
            })
        }
        menu.addAction(UIAlertAction(title: "Hủy", style: .cancel))
        present(menu, animated: true)
    }
    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        "Áp dụng khi bấm Chơi. Tầm nhìn mở rộng khác khoảng cách vẽ thông thường; tăng mức này sẽ tăng lượng cảnh cần nạp. Với iPhone 4 GB, thử 0% trước. Extended Texture Budget ở bản port này điều chỉnh ngân sách video mà engine dùng cho streaming, không cấp thêm RAM cho iOS. Ngân sách quá thấp có thể làm thiếu texture; quá cao có thể khiến app hết RAM. Thử 512 hoặc 640 MiB, giữ cùng tuyến đường để so sánh rồi tăng từng mức nếu cần. Tự động khôi phục hành vi bản nền."
    }
}
