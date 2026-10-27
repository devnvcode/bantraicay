import Foundation
#if canImport(FoundationXML)
import FoundationXML
#endif
@main struct SoundTest {
 static func main() throws {
  let fm=FileManager.default,fixture=URL(fileURLWithPath:CommandLine.arguments[1])
  let stage=try SoundInstaller.stage([fixture]);defer{try? fm.removeItem(at:stage)}
  let (plan,profiles)=try SoundInstaller.build(stage)
  precondition(profiles==["ta103ninjah2r"])
  let archive=try RPFArchive(url:plan.items.first!.source)
  let rel=archive.entries.first{$0.path=="audioconfig/ta103ninjah2r_sounds.dat54.rel"}!
  let native=archive.entries.first{$0.path=="audioconfig/ta103ninjah2r_sounds.dat54"}!
  let original=try archive.binary(rel),converted=try archive.binary(native)
  let expected=try ModAudio.nativeSoundData(original);precondition(converted==expected);precondition(original != converted)
  let bank=archive.entries.first{$0.path=="sfx/dlc_ta103ninjah2r/ta103ninjah2r.awc"}!
  let got=try archive.binary(bank),source=try Data(contentsOf:fixture.appendingPathComponent("sfx/dlc_ta103ninjah2r/ta103ninjah2r.awc"));precondition(got==source)
  for name in ["setup2.xml","content.xml"] {
   let e=archive.entries.first{$0.path==name}!;let xml=try archive.metadata(e);precondition(XMLParser(data:xml).parse())
  }
  // A Singleplayer RPF often nests sfx/dlc_bank.rpf. Keep the parent bank name.
  let nested=try VehicleInstaller.pack(["ta103ninjah2r.awc":source])
  let game=try Data(contentsOf:fixture.appendingPathComponent("audioconfig/ta103ninjah2r_game.dat151.rel"))
  let single=try VehicleInstaller.pack(["sfx/dlc_ta103ninjah2r.rpf":nested,"audioconfig/ta103ninjah2r_game.dat151.rel":game,"audioconfig/ta103ninjah2r_sounds.dat54.rel":original])
  let root=fm.temporaryDirectory.appendingPathComponent("sound-nested-"+UUID().uuidString)
  try fm.createDirectory(at:root,withIntermediateDirectories:true);defer{try? fm.removeItem(at:root)}
  let singleFile=root.appendingPathComponent("dlc.rpf");try single.write(to:singleFile)
  let nestedStage=try SoundInstaller.stage([singleFile]);defer{try? fm.removeItem(at:nestedStage)}
  let (nestedPlan,nestedProfiles)=try SoundInstaller.build(nestedStage)
  precondition(nestedProfiles==profiles)
  let nestedArchive=try RPFArchive(url:nestedPlan.items.first!.source)
  let nestedBank=nestedArchive.entries.first{$0.path==bank.path}!
  let nestedBytes=try nestedArchive.binary(nestedBank);precondition(nestedBytes==source)
  let bad=stage.appendingPathComponent("audioconfig");try fm.createDirectory(at:bad,withIntermediateDirectories:true)
  try Data([54,0,0,0]).write(to:bad.appendingPathComponent("broken_sounds.dat54.rel"))
  do{_ = try SoundInstaller.build(stage);fatalError("truncated REL accepted")}catch is ModError{}
  // Metadata repair must preserve every nested archive byte and remain idempotent.
  let modRoot=ModInstaller.modsFolder.appendingPathComponent("mulone-test-"+UUID().uuidString)
  let packDir=modRoot.appendingPathComponent("update/x64/dlcpacks/test")
  try fm.createDirectory(at:packDir,withIntermediateDirectories:true);defer{try? fm.removeItem(at:modRoot)}
  let vehicleXML=Data("<CVehicleModelInfo__InitDataList><InitDatas><Item><modelName>sh150</modelName></Item></InitDatas><txdRelationships><Item><parent>vehicles_biker_shared</parent><child>sh150</child></Item></txdRelationships></CVehicleModelInfo__InitDataList>".utf8)
  let vehiclePack=try VehicleInstaller.pack(["data/vehicles.meta":vehicleXML,"x64/vehicles.rpf":single])
  let vehicleURL=packDir.appendingPathComponent("dlc.rpf");try vehiclePack.write(to:vehicleURL)
  let manifestURL=root.appendingPathComponent("manifest.json")
  try JSONSerialization.data(withJSONObject:["files":[["x64/vehshare.ytd",16,0]]]).write(to:manifestURL)
  ModVehicleSupport.prepare(manifestPath:manifestURL.path,log:{print($0)})
  let repaired=try RPFArchive(url:vehicleURL)
  let meta=try repaired.metadata(repaired.entries.first{$0.path=="data/vehicles.meta"}!)
  precondition(String(data:meta,encoding:.utf8)!.contains("<parent>vehshare</parent>"))
  let nestedEntry=repaired.entries.first{$0.path=="x64/vehicles.rpf"}!
  precondition(repaired.bytes.subdata(in:nestedEntry.offset..<(nestedEntry.offset+nestedEntry.size))==single)
  let backup=try Data(contentsOf:packDir.appendingPathComponent(".dlc.rpf.mulone-original"));precondition(backup==vehiclePack)
  let first=try Data(contentsOf:vehicleURL)
  ModVehicleSupport.prepare(manifestPath:manifestURL.path,log:{print($0)})
  let second=try Data(contentsOf:vehicleURL);precondition(first==second)
  precondition(ModVehicleSupport.vehicles().contains("sh150"))
  // Reproduce tester's duplicate raw + converted st76cb4t mounts.
  let rawURL=fixture.deletingLastPathComponent().appendingPathComponent("st76cb4t/dlc.rpf")
  let stStage=try SoundInstaller.stage([rawURL]);defer{try? fm.removeItem(at:stStage)}
  let (stPlan,stProfiles)=try SoundInstaller.build(stStage);precondition(stProfiles==["st76cb4t"])
  let convertedRoot=try ModInstaller.install(stPlan);defer{try? fm.removeItem(at:convertedRoot)}
  let rawRoot=ModInstaller.modsFolder.appendingPathComponent("raw-st76-test")
  let rawPackDir=rawRoot.appendingPathComponent("update/x64/dlcpacks/st76cb4t")
  try fm.createDirectory(at:rawPackDir,withIntermediateDirectories:true)
  try fm.copyItem(at:rawURL,to:rawPackDir.appendingPathComponent("dlc.rpf"))
  defer{try? fm.removeItem(at:rawRoot);try? fm.removeItem(atPath:rawRoot.path+".off")}
  SoundAssignments.resolveDuplicatePacks(log:{print($0)})
  precondition(!fm.fileExists(atPath:rawRoot.path));precondition(fm.fileExists(atPath:rawRoot.path+".off"))
  precondition(fm.fileExists(atPath:convertedRoot.path))
  print("PASS: actual H2R FiveM folder import, native REL54 alias, unchanged AWC, valid DLC XML and corrupt input rejection")
 }
}
