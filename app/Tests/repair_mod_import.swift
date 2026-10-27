import Foundation
import Darwin
#if canImport(FoundationXML)
import FoundationXML
#endif
@main struct RepairMods {
 static func check(_ ok:Bool,_ message:String)throws{if !ok{throw ModError(message)}}
 static func main(){do{try run()}catch{fputs("FAIL: \(error)\n",stderr);exit(1)}}
 static func run()throws{
 let fm=FileManager.default,root=URL(fileURLWithPath:CommandLine.arguments[1])
 let game=root.appendingPathComponent("game")
 try fm.createDirectory(at:game.appendingPathComponent("common/data/levels/gta5"),withIntermediateDirectories:true)
 try fm.createDirectory(at:game.appendingPathComponent("x64/levels/gta5"),withIntermediateDirectories:true)
 try fm.copyItem(at:root.appendingPathComponent("vehicles.meta"),to:game.appendingPathComponent("common/data/levels/gta5/vehicles.meta"))
 // Routing fixture only: actual map/rendering is not represented by this placeholder.
 try Data().write(to:game.appendingPathComponent("x64/levels/gta5/vehicles.rpf"))
 var index=ModInstaller.GameIndex();ModInstaller.addStockVehicleIndex(game,to:&index)
 let thai=try ModInstaller.unpack(root.appendingPathComponent("thai.rar"));defer{try? fm.removeItem(at:thai)}
 let plan=ModInstaller.plan(folder:thai,name:"repair-thai",index:index)
 try check(plan.items.count==3,"Thai RAR must route all 3 resources, including +hi.ytd: \(plan.skipped)")
 try check(plan.items.allSatisfy{$0.target.hasPrefix("x64/levels/gta5/vehicles.rpf/")},"Thai resource left in pending instead of actual vehicle archive")
 try check(Set(plan.items.map{($0.target as NSString).lastPathComponent})==Set(["bati2.yft","bati2.ytd","bati2+hi.ytd"]),"Thai target names changed")
 let installed=try ModInstaller.install(plan);defer{try? fm.removeItem(at:installed)}
 for item in plan.items{let source=try Data(contentsOf:item.source);let expected=try ModResource.normalize(source,name:item.source.lastPathComponent);try check(expected==Data(contentsOf:installed.appendingPathComponent(item.target)),"Replace payload conversion differs")}
 // Failed reinstall must not destroy the working install.
 var broken=ModInstaller.Plan(name:plan.name);broken.items=[.init(source:root.appendingPathComponent("missing.yft"),target:"x64/levels/gta5/vehicles.rpf/bati2.yft",note:"")]
 do{_ = try ModInstaller.install(broken);throw ModError("Missing file accepted")}catch{try check(fm.fileExists(atPath:installed.appendingPathComponent(plan.items[0].target).path),"Failed install destroyed previous mod")}
 print("PASS Thai RAR: 3 validated resource replacements, failed reinstall rollback")
 for (file,model) in [("audi.zip","sgr8lmsevo"),("vf8.zip","vf8")]{
  let stage=try ModInstaller.unpack(root.appendingPathComponent(file));defer{try? fm.removeItem(at:stage)}
  var expanded=index
  // Simulate known global metadata before the runtime index exists: FiveM duplicates must not overwrite it.
  expanded.byName["vehicles.meta"]=["common/data/levels/gta5/vehicles.meta"]
  expanded.byName["handling.meta"]=["common/data/handling.meta"]
  let plan=ModInstaller.plan(folder:stage,name:"repair-"+model,index:expanded)
  try check(plan.items.count==1 && plan.items[0].target.hasSuffix("/dlc.rpf"),"DLC imported alongside duplicate loose/global files: \(file)")
  let archive=try RPFArchive(url:plan.items[0].source);var resources=Set<String>()
  func walk(_ a:RPFArchive,_ depth:Int)throws{
   try check(depth<8,"Nested depth")
   for e in a.entries{
    if e.path.hasSuffix(".rpf"){try walk(a.nested(e),depth+1)}
    else if let version=e.resourceVersion{
     if e.path.hasSuffix(".yft"){try check(version==162,"Unsupported YFT version");resources.insert((e.path as NSString).lastPathComponent)}
    }
   }
  }
  try walk(archive,0)
  try check(resources.contains(model+".yft"),"Declared model has no geometry resource")
  let metas=archive.entries.filter{$0.path.hasSuffix("vehicles.meta")};try check(!metas.isEmpty,"Missing vehicle registration")
  for e in metas{let data=try archive.metadata(e);try check(XMLParser(data:data).parse(),"Invalid vehicle XML");try check(String(data:data,encoding:.utf8)?.contains("<modelName>"+model+"</modelName>")==true,"Wrong registered model")}
  let target=try ModInstaller.install(plan);defer{try? fm.removeItem(at:target)}
  try check(Data(contentsOf:plan.items[0].source)==Data(contentsOf:target.appendingPathComponent(plan.items[0].target)),"DLC bytes changed during install")
  print("PASS \(file): one DLC, registered model and real Legacy geometry present; archive byte-preserved")
 }
 print("NOT TESTED: actual model rendering on iPhone; disk/import tests are not a visible-spawn claim")
 }
}
