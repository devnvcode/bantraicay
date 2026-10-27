import Foundation
import Darwin
#if canImport(FoundationXML)
import FoundationXML
#endif

@main struct OwnerMods {
 static func main() throws {
  setbuf(stdout,nil)
  let fm=FileManager.default,root=URL(fileURLWithPath:CommandLine.arguments[1])
  func files(_ folder:URL,_ ext:String)->[URL] {
   (fm.enumerator(at:folder,includingPropertiesForKeys:[.isRegularFileKey])?.allObjects as? [URL] ?? []).filter{$0.pathExtension.lowercased()==ext}.sorted{$0.path<$1.path}
  }
  for name in ["laferrari.rar","sh150.zip","vision.zip"] {
   let stage=try ModInstaller.unpack(root.appendingPathComponent("cars/"+name));defer{try? fm.removeItem(at:stage)}
   let plan=ModInstaller.plan(folder:stage,name:name,index:.init())
   precondition(plan.items.count==1 && plan.items[0].target.hasSuffix("/dlc.rpf"),"Vehicle rejected: \(name): \(plan.skipped)")
   let original=try RPFArchive(url:plan.items[0].source)
   let installed=try ModInstaller.install(plan);defer{try? fm.removeItem(at:installed)}
   let manifest=root.appendingPathComponent("owner-manifest.json")
   try JSONSerialization.data(withJSONObject:["files":[[Any]]()]).write(to:manifest)
   ModVehicleSupport.prepare(manifestPath:manifest.path,log:{print($0)})
   let r=try RPFArchive(url:installed.appendingPathComponent(plan.items[0].target))
   let metas=r.entries.filter{$0.path.hasSuffix("vehicles.meta")}
   precondition(!metas.isEmpty)
   for e in metas { let data=try r.metadata(e);precondition(XMLParser(data:data).parse()) }
   for e in original.entries where e.path.hasSuffix(".rpf") {
    let after=r.entries.first{$0.path==e.path}!
    precondition(e.size==after.size && original.bytes[e.offset..<(e.offset+e.size)]==r.bytes[after.offset..<(after.offset+after.size)],"Nested vehicle assets changed: \(e.path)")
   }
   print("PASS owner vehicle archive: \(name), \(r.entries.count) outer entries")
  }
  let sound=try SoundInstaller.stage([root.appendingPathComponent("sound/engines.zip")]);defer{try? fm.removeItem(at:sound)}
  precondition(SoundInstaller.isSoundOnly(sound),"General importer must detect this sound-only DLC")
  let (soundPlan,profiles)=try SoundInstaller.build(sound)
  let expected:Set<String>=["brabus850","toysupmk4","lambov10","rb26dett","rotary7","musv8","m297zonda","m158huayra","k20a","gt3flat6","predatorv8","p60b40","shonen"]
  precondition(Set(profiles)==expected,"Missing engine profiles: \(profiles)")
  let audio=try RPFArchive(url:soundPlan.items[0].source)
  for profile in profiles {
   let original=audio.entries.first{$0.path=="audioconfig/\(profile)_sounds.dat54.rel"}!
   let native=audio.entries.first{$0.path=="audioconfig/\(profile)_sounds.dat54"}!
   let input=try audio.binary(original),output=try audio.binary(native)
   let converted=try ModAudio.nativeSoundData(input)
   precondition(output == converted && output != input)
   precondition(audio.entries.contains{$0.path.hasPrefix("sfx/dlc_\(profile)/") && $0.path.hasSuffix(".awc")})
  }
  for name in ["setup2.xml","content.xml"] { let data=try audio.metadata(audio.entries.first{$0.path==name}!);precondition(XMLParser(data:data).parse()) }
  print("PASS owner Aquaphobic RAR-with-ZIP-extension: all \(profiles.count) profiles, converted REL54, AWC banks and mounts")

  let skins=try ModInstaller.unpack(root.appendingPathComponent("skins/nier.rar"));defer{try? fm.removeItem(at:skins)}
  let plan=try SkinInstaller.plan(folder:skins,name:"owner-nier")
  let list=plan.items.first{$0.target=="skin-models.json"}!
  let models=try JSONSerialization.jsonObject(with:Data(contentsOf:list.source)) as! [String]
  precondition(models.contains("2b") && models.contains("a2") && models.count>=12)
  for item in plan.items where item.target.hasSuffix("/dlc.rpf") {
   let pack=try RPFArchive(url:item.source)
   for name in ["setup2.xml","content.xml","peds.meta"] { let data=try pack.metadata(pack.entries.first{$0.path==name}!);precondition(XMLParser(data:data).parse()) }
   let inner=try pack.nested(pack.entries.first{$0.path=="peds.rpf"}!)
   precondition(inner.entries.count==4)
   for entry in inner.entries {
    let restored=try inner.resourceData(entry)
    let original=files(skins,(entry.path as NSString).pathExtension).first{$0.lastPathComponent.lowercased()==entry.path}!
    let source=try Data(contentsOf:original)
    let expected = try ModResource.normalize(source,name:entry.path)
    precondition(restored==expected,"Skin payload conversion differs: \(entry.path)")
   }
  }
  // Multi-select the actual four 2B files, with each in its own picker staging directory.
  let quartet=files(skins,"ydd").first{$0.lastPathComponent.lowercased()=="2b.ydd"}!.deletingLastPathComponent()
  let four=try fm.contentsOfDirectory(at:quartet,includingPropertiesForKeys:nil).filter{$0.deletingPathExtension().lastPathComponent.lowercased()=="2b" && SkinInstaller.extensions.contains($0.pathExtension.lowercased())}
  let selected=try ModInstaller.unpackMany(four);defer{try? fm.removeItem(at:selected)}
  let selectedPlan=try SkinInstaller.plan(folder:selected,name:"selected-2b")
  precondition(selectedPlan.items.count==2)
  let partial=try ModInstaller.unpackMany(Array(four.prefix(3)));defer{try? fm.removeItem(at:partial)}
  do { _=try SkinInstaller.plan(folder:partial,name:"incomplete");fatalError("Incomplete ped accepted") } catch is ModError {}
  // The same quartet inside a real ZIP follows the identical installer.
  let zipped=root.appendingPathComponent("skins/2b.zip")
  let task=Process();task.executableURL=URL(fileURLWithPath:"/usr/bin/zip");task.currentDirectoryURL=quartet
  task.arguments=["-q",zipped.path]+four.map(\.lastPathComponent);try task.run();task.waitUntilExit();precondition(task.terminationStatus==0)
  let zipStage=try ModInstaller.unpack(zipped);defer{try? fm.removeItem(at:zipStage)}
  let zipPlan=try SkinInstaller.plan(folder:zipStage,name:"zip-2b");precondition(zipPlan.items.map(\.target)==selectedPlan.items.map(\.target))
  let installed=try ModInstaller.install(plan);defer{try? fm.removeItem(at:installed)}
  let menyoo=try ModInstaller.unpack(root.appendingPathComponent("menyoo/menyoo.zip"));defer{try? fm.removeItem(at:menyoo)}
  let owned=ModInstaller.modsFolder.appendingPathComponent(BundledMenyoo.name)
  precondition(!fm.fileExists(atPath:owned.path));try fm.createDirectory(at:owned.deletingLastPathComponent(),withIntermediateDirectories:true)
  try fm.moveItem(at:menyoo,to:owned);defer{try? fm.removeItem(at:owned)}
  let untouched=try Data(contentsOf:owned.appendingPathComponent("menyooStuff/PedList.xml"))
  try SkinInstaller.updateMenyooList()
  let afterUnowned=try Data(contentsOf:owned.appendingPathComponent("menyooStuff/PedList.xml"))
  precondition(untouched==afterUnowned,"Unowned Menyoo package modified")
  try "2".write(to:owned.appendingPathComponent(".newgen-menyoo"),atomically:true,encoding:.utf8)
  try SkinInstaller.updateMenyooList();try SkinInstaller.updateMenyooList()
  let pedList=try String(contentsOf:owned.appendingPathComponent("menyooStuff/PedList.xml"),encoding:.utf8)
  precondition(pedList.components(separatedBy:"<!-- NewGen addon peds -->").count==2)
  for model in models {precondition(pedList.contains("name=\"\(model)\""))}
  precondition(XMLParser(data:Data(pedList.utf8)).parse())
  print("PASS owner NIER: \(models.count) registered addon models, byte-identical resources, ZIP/multiselect and idempotent Menyoo list")

  StreamingSettings.distanceScale="0.2";precondition(StreamingSettings.distanceScale=="0.2")
  StreamingSettings.distanceScale="999";precondition(StreamingSettings.distanceScale==nil)
  StreamingSettings.textureBudgetMB=512;StreamingSettings.applyTextureBudget();precondition(String(cString:getenv("PG_VIDEO_MEMORY_MB")!)=="512")
  StreamingSettings.textureBudgetMB=0;StreamingSettings.applyTextureBudget();precondition(getenv("PG_VIDEO_MEMORY_MB")==nil)
  print("PASS boot streaming preferences: valid values, invalid value rejected, automatic restores environment")
 }
}
