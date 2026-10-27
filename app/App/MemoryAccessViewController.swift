import UIKit
import os
/// Reports actual process headroom; never pretends a UI action grants an iOS entitlement.
final class MemoryAccessViewController: UITableViewController {
 private var tier=DeviceTier.probe()
 private let running:Bool
 init(engineRunning:Bool){running=engineRunning;super.init(style:.insetGrouped);title="RAM và quyền bộ nhớ"}
 required init?(coder:NSCoder){fatalError("init(coder:) has not been implemented")}
 override func viewDidLoad(){super.viewDidLoad();Theme.styleSettingsTable(tableView)}
 override func numberOfSections(in tableView:UITableView)->Int{1}
 override func tableView(_ tableView:UITableView,numberOfRowsInSection section:Int)->Int{9}
 override func tableView(_ tableView:UITableView,cellForRowAt indexPath:IndexPath)->UITableViewCell{
  let cell=UITableViewCell(style:.subtitle,reuseIdentifier:nil);var c=cell.defaultContentConfiguration()
  switch indexPath.row {
  case 0:
   c.text="RAM iOS đang cho phép (ước tính)";c.secondaryText="\(tier.memoryLimitMB) MiB · RAM máy \(tier.physicalMemoryMB) MiB. Thay đổi theo thiết bị, chữ ký và áp lực bộ nhớ."
  case 1:
   c.text="Quyền tăng RAM trên bản đã cài"
   c.secondaryText=DeviceTier.increasedMemoryGranted.map{$0 ? "Có entitlement tăng RAM trong chữ ký.":"Không thấy entitlement tăng RAM trong chữ ký. Cần kiểm tra khi ký lại."} ?? "Không đọc được entitlement của bản đã cài."
  case 2:
   c.text="Kiểm tra lại quyền và RAM";c.secondaryText="Đo lại bộ nhớ hiện tại; không cấp quyền giả và không ép cấp phát RAM.";cell.accessoryType = .disclosureIndicator
  case 3:
   let cap=GameSettings.shared.memoryCapMB
   c.text="Dùng toàn bộ RAM iOS cho phép";c.secondaryText=cap==0 ? "Đang tự động: không đặt cap riêng trong app. Vùng địa chỉ engine tự chọn khoảng 4–7 GiB theo ngân sách; KHÁC RAM thật của app.":"Cap hiện tại \(cap) MiB. Nhấn để về Tự động; áp dụng lần mở game tiếp theo."
   cell.accessoryType = cap==0 ? .checkmark:.disclosureIndicator
  case 4:
   c.text="Xuất file quyền RAM để ký lại";c.secondaryText="Chia sẻ plist entitlement cho công cụ ký hỗ trợ. Chứng chỉ/profile phải cho phép; không có hộp thoại xin RAM của iOS.";cell.accessoryType = .disclosureIndicator
  case 6:
   c.text="Thu hồi theo di chuyển"
   c.secondaryText="Ưu tiên giữ tài nguyên gần nhân vật; dọn theo khoảng cách bên dưới hoặc chuyển cảnh. Chỉ dọn khẩn cấp khi RAM trống dưới nửa mức dự phòng, tối đa 16 MB/đợt và 2 đợt/giây. Tắt để về chính sách R005; áp dụng lần mở app sau."
   let control=UISwitch();control.isOn=StreamingSettings.distanceStreaming;control.addTarget(self,action:#selector(distanceStreamingChanged(_:)),for:.valueChanged);cell.accessoryView=control
  case 7:
   c.text="RAM dự phòng khi thu hồi theo di chuyển"
   c.secondaryText="Tự động: máy ≤4 GB: 128 MB; 6 GB–dưới 8 GB: 192 MB; từ 8 GB: 256 MB. App bị iOS giới hạn thấp sẽ tự giảm mức dự phòng. Không dùng dải chặn tăng trưởng lớn của R005. Áp dụng lần mở app sau."
   let button=UIButton(type:.system);button.setTitle(reserveTitle(StreamingSettings.reserveProfileMB)+" ▾",for:.normal);button.showsMenuAsPrimaryAction=true
   button.menu=UIMenu(children:StreamingSettings.reserveValues.map{value in UIAction(title:reserveTitle(value),state:StreamingSettings.reserveProfileMB==value ? .on:.off){[weak self] _ in StreamingSettings.reserveProfileMB=value;self?.tableView.reloadData()}});button.sizeToFit();cell.accessoryView=button
  case 8:
   c.text="Khoảng cách cho phép dọn"
   c.secondaryText="Mặc định 80 m, bằng một nửa R006. Đây là điều kiện cho phép engine dọn tài nguyên không còn cần, không xoá mọi vật thể ngoài bán kính. Áp dụng lần mở app sau."
   let button=UIButton(type:.system);button.setTitle("\(StreamingSettings.reclaimDistance) m ▾",for:.normal);button.showsMenuAsPrimaryAction=true
   button.menu=UIMenu(children:StreamingSettings.reclaimDistances.map{value in UIAction(title:"\(value) m",state:StreamingSettings.reclaimDistance==value ? .on:.off){[weak self] _ in StreamingSettings.reclaimDistance=value;self?.tableView.reloadData()}});button.sizeToFit();cell.accessoryView=button
  default:
   c.text="Vì sao không tự nâng lên 5 GB?";c.secondaryText="App đã yêu cầu increased-memory-limit và extended-virtual-addressing trong IPA. iOS quyết định mức thực tế. Nút bên trên không thay chữ ký, không bypass giới hạn và không đổi giới hạn engine để tránh crash."
  }
  c.textProperties.color=Theme.text;c.secondaryTextProperties.color=Theme.secondaryText;c.secondaryTextProperties.numberOfLines=0;cell.contentConfiguration=c;return cell
 }
 private func reserveTitle(_ value:Int)->String{
  switch value{case -1:return "Tự động (\(StreamingSettings.selectedReserveMB) MB)";case 0:return "Không dự phòng (R006)";case 128:return "Máy ≤4 GB · 128 MB";case 192:return "Máy 6–<8 GB · 192 MB";default:return "Máy ≥8 GB · 256 MB"}
 }
 @objc private func distanceStreamingChanged(_ sender:UISwitch){StreamingSettings.distanceStreaming=sender.isOn}
 override func tableView(_ tableView:UITableView,didSelectRowAt indexPath:IndexPath){
  tableView.deselectRow(at:indexPath,animated:true)
  if indexPath.row==2 {tier=DeviceTier.probe();GameSettings.shared.tier=tier;tableView.reloadData()}
  if indexPath.row==3 {GameSettings.shared.memoryCapMB=0;tableView.reloadData();if running{let a=UIAlertController(title:"Đã chọn RAM tự động",message:"Áp dụng lần mở game tiếp theo. Quyền iOS không thay đổi trong phiên đang chạy.",preferredStyle:.alert);a.addAction(UIAlertAction(title:"OK",style:.default));present(a,animated:true)}}
  if indexPath.row==4 {
   let values:[String:Bool]=["com.apple.developer.kernel.increased-memory-limit":true,"com.apple.developer.kernel.extended-virtual-addressing":true]
   let url=FileManager.default.temporaryDirectory.appendingPathComponent("Mugoat-NewGen-RAM.entitlements")
   do{let data=try PropertyListSerialization.data(fromPropertyList:values,format:.xml,options:0);try data.write(to:url,options:.atomic)
    let share=UIActivityViewController(activityItems:[url],applicationActivities:nil);share.popoverPresentationController?.sourceView=tableView.cellForRow(at:indexPath);present(share,animated:true)
   }catch{let a=UIAlertController(title:"Không thể xuất file",message:error.localizedDescription,preferredStyle:.alert);a.addAction(UIAlertAction(title:"OK",style:.default));present(a,animated:true)}
  }
 }
}
