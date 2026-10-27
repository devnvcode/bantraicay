import UIKit
struct DeviceTier {
 var memoryLimitMB=2348
 var physicalMemoryMB=4096
 static var increasedMemoryGranted:Bool? {true}
 static func probe()->DeviceTier{DeviceTier()}
}
final class GameSettings {
 static let shared=GameSettings()
 var memoryCapMB=0
 var tier=DeviceTier()
}
enum Theme {
 static var text:UIColor{.label}
 static var secondaryText:UIColor{.secondaryLabel}
 static func styleSettingsTable(_ table:UITableView){}
}
