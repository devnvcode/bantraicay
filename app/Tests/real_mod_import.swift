import Foundation
import Darwin
#if canImport(FoundationXML)
import FoundationXML
#endif
@main struct RealMods {
 static func main() throws {
  setbuf(stdout,nil)
  let fm=FileManager.default,root=URL(fileURLWithPath:CommandLine.arguments[1])
  let game=fm.temporaryDirectory.appendingPathComponent("game-test-"+UUID().uuidString)
  try fm.createDirectory(at:game.appendingPathComponent("common/data/levels/gta5"),withIntermediateDirectories:true)
  try fm.createDirectory(at:game.appendingPathComponent("x64/levels/gta5"),withIntermediateDirectories:true)
  defer{try? fm.removeItem(at:game)}
  try "<modelName>tailgater</modelName>".write(to:game.appendingPathComponent("common/data/levels/gta5/vehicles.meta"),atomically:true,encoding:.utf8)
  try Data().write(to:game.appendingPathComponent("x64/levels/gta5/vehicles.rpf"))
  var index=ModInstaller.GameIndex();ModInstaller.addStockVehicleIndex(game,to:&index)
  let files=try fm.contentsOfDirectory(at:root.appendingPathComponent("maybach"),includingPropertiesForKeys:nil)
  precondition(files.count==3)
  let stage=try ModInstaller.unpackMany(files);defer{try? fm.removeItem(at:stage)}
  let plan=ModInstaller.plan(folder:stage,name:"test-maybach",index:index)
  precondition(plan.items.count==3 && plan.skipped.isEmpty,"Maybach 3-file import failed: \(plan.skipped)")
  precondition(Set(plan.items.map{$0.target})==Set(["tailgater.yft","tailgater_hi.yft","tailgater.ytd"].map{"x64/levels/gta5/vehicles.rpf/"+$0}))
  let zipStage=try ModInstaller.unpack(root.appendingPathComponent("maybach.zip"));defer{try? fm.removeItem(at:zipStage)}
  let zipPlan=ModInstaller.plan(folder:zipStage,name:"test-maybach-zip",index:index)
  precondition(Set(zipPlan.items.map{$0.target})==Set(plan.items.map{$0.target}))
  for (dir,expected) in [("hammer-legacy",1),("hammer-enhanced",0),("revuelto",1)] {
   let plan=ModInstaller.plan(folder:root.appendingPathComponent(dir),name:dir,index:index)
   precondition(plan.items.count==expected,"Vehicle compatibility failed: \(dir), \(plan.skipped)")
   if expected==0{precondition(plan.unsupported)}
   else{
    let r=try RPFArchive(url:plan.items[0].source)
    let meta=r.entries.filter{$0.path.hasSuffix("vehicles.meta")}
    precondition(!meta.isEmpty)
    for entry in meta{let b=try r.metadata(entry);precondition(String(data:b,encoding:.utf8)?.contains("<modelName>")==true)}
   }
   print("PASS: actual vehicle package \(dir), accepted=\(expected==1)")
  }
  let audio=try fm.contentsOfDirectory(at:root.appendingPathComponent("audio"),includingPropertiesForKeys:nil).sorted{$0.path<$1.path}
  for url in audio {
   print("Testing audio: \(url.lastPathComponent)")
   let stage=try SoundInstaller.stage([url]);defer{try? fm.removeItem(at:stage)}
   let (plan,profiles)=try SoundInstaller.build(stage)
   precondition(!profiles.isEmpty && plan.items.count>0)
   for item in plan.items where item.source.pathExtension=="rpf" {
    let r=try RPFArchive(url:item.source)
    precondition(r.entries.contains{$0.path.hasSuffix(".awc") || $0.path.hasSuffix(".rpf")})
    precondition(r.entries.contains{$0.path.hasSuffix("_sounds.dat54")})
   }
   print("PASS: actual sound archive \(url.lastPathComponent), \(profiles.count) profiles")
  }
  EnginePaths.testRoot=game.path
  let profile=SaveImport.folder.deletingLastPathComponent().appendingPathComponent("pc_settings.bin")
  try fm.createDirectory(at:profile.deletingLastPathComponent(),withIntermediateDirectories:true)
  let original=Data("keep-player-settings".utf8);try original.write(to:profile)
  switch SaveImport.importSave(from:root.appendingPathComponent("save")) {
   case .imported:break
   case .failed(let message):fatalError(message)
  }
  let keptProfile=try Data(contentsOf:profile);precondition(keptProfile==original)
  precondition(fm.fileExists(atPath:SaveImport.folder.appendingPathComponent("SGTA50000").path))
  print("PASS: real save bundle imported without replacing pc_settings.bin")
 }
}
