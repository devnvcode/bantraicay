import Foundation
/// Only mod metadata is inspected, before engine startup. No model/texture conversion.
enum ModVehicleSupport {
 static func vehicles()->[String] {
  ModInstaller.installed().filter{$0.enabled}.flatMap{mod -> [String] in
   guard let b=try? Data(contentsOf:mod.url.appendingPathComponent(".mulone-vehicles.json")),let v=try? JSONSerialization.jsonObject(with:b) as? [String] else{return []}
   return v.filter{ModPack.validName($0)}
  }.sorted()
 }
 static func prepare(manifestPath:String,log:(String)->Void) {
  do {
   let fm=FileManager.default
   guard let b=try? Data(contentsOf:URL(fileURLWithPath:manifestPath)),let manifest=try? JSONSerialization.jsonObject(with:b) as? [String:Any],let rows=manifest["files"] as? [[Any]] else{return}
   var known=Set(rows.compactMap{$0.first as? String}.filter{$0.lowercased().hasSuffix(".ytd")}.map{URL(fileURLWithPath:$0).deletingPathExtension().lastPathComponent.lowercased()})
   known.insert("vehshare")
   var archives:[(URL,RPFArchive,URL)]=[]
   var names:[URL:Set<String>]=[:]
   func textures(_ r:RPFArchive,_ depth:Int) {
    guard depth<5 else{return}
    for e in r.entries {
     if e.path.hasSuffix(".ytd"){known.insert(URL(fileURLWithPath:e.path).deletingPathExtension().lastPathComponent)}
     if e.path.hasSuffix(".rpf"),let child=try? r.nested(e){textures(child,depth+1)}
    }
   }
   let mods=ModInstaller.installed().filter{$0.enabled && !$0.name.hasPrefix("_newgen_story_") && !fm.fileExists(atPath:$0.url.appendingPathComponent("skin-models.json").path)}
   for mod in mods {
    guard let walk=fm.enumerator(at:mod.url,includingPropertiesForKeys:nil,options:.skipsHiddenFiles) else{continue}
    for case let url as URL in walk {
     if url.pathExtension.lowercased()=="yft" {
      var model=url.deletingPathExtension().lastPathComponent.lowercased().replacingOccurrences(of:"-",with:"_");if model.hasSuffix("_hi"){model=String(model.dropLast(3))}
      if ModPack.validName(model){names[mod.url,default:[]].insert(model)}
     }
     guard url.lastPathComponent.lowercased()=="dlc.rpf" else{continue}
     do{let r=try RPFArchive(url:url);textures(r,0);archives.append((url,r,mod.url))}catch{log("Vehicle metadata scan: \(error.localizedDescription)")}
    }
   }
   let parentRE=try! NSRegularExpression(pattern:"<parent>\\s*([^<]+)\\s*</parent>")
   let modelRE=try! NSRegularExpression(pattern:"<modelName>\\s*([^<]+)\\s*</modelName>")
   let layoutRE=try! NSRegularExpression(pattern:"<layout>\\s*([^<]+)\\s*</layout>")
   let audioRE=try! NSRegularExpression(pattern:"<audioNameHash>\\s*([^<]*)\\s*</audioNameHash>")
   let sitRE=try! NSRegularExpression(pattern:"<sitInBike\\s+value=[\"']([^\"']*)[\"']\\s*/>")
   for (url,r,root) in archives {
    do {
     var out=r.bytes,changed=false
     for e in r.entries where e.path.hasSuffix("vehicles.meta") {
      let data=try r.metadata(e)
      guard var text=String(data:data,encoding:.utf8) else{continue}
      // Some PC packs use a long run of dashes as an XML comment. XML forbids
      // internal "--"; retain a valid separator without touching any game fields.
      text=text.replacingOccurrences(of:"<!-{3,}>",with:"<!-- separator -->",options:.regularExpression)

      // Pass 1: Extract model names
      var ns=text as NSString
      for m in modelRE.matches(in:text,range:NSRange(location:0,length:ns.length)) {
       if m.numberOfRanges > 1 && m.range(at:1).location != NSNotFound {
        let name=ns.substring(with:m.range(at:1)).trimmingCharacters(in:.whitespacesAndNewlines).lowercased().replacingOccurrences(of:"-",with:"_")
        if ModPack.validName(name){names[root,default:[]].insert(name)}
       }
      }

      // Pass 2: Missing texture parent -> vehshare
      ns=text as NSString
      for m in parentRE.matches(in:text,range:NSRange(location:0,length:ns.length)).reversed() {
       if m.numberOfRanges > 1 && m.range(at:1).location != NSNotFound {
        let parent=ns.substring(with:m.range(at:1)).trimmingCharacters(in:.whitespacesAndNewlines).lowercased()
        if !known.contains(parent) {
         let curNS=text as NSString
         if m.range.location != NSNotFound && m.range.location + m.range.length <= curNS.length {
          text=curNS.replacingCharacters(in:m.range,with:"<parent>vehshare</parent>")
          log("Vehicle compatibility: missing texture parent \(parent) -> vehshare in \(url.lastPathComponent)/\(e.path).")
         }
        }
       }
      }

      // Pass 3: Normalize mod vehicle seating layouts so rider/driver skeleton aligns properly on top of seat
      ns=text as NSString
      for m in layoutRE.matches(in:text,range:NSRange(location:0,length:ns.length)).reversed() {
       if m.numberOfRanges > 1 && m.range(at:1).location != NSNotFound {
        let layout=ns.substring(with:m.range(at:1)).trimmingCharacters(in:.whitespacesAndNewlines).uppercased()
        let stdLayouts: Set<String> = ["LAYOUT_STANDARD", "LAYOUT_STD_MOTORBIKE", "LAYOUT_BICYCLE", "LAYOUT_STD_BOAT", "LAYOUT_STD_HELI", "LAYOUT_STD_PLANE", "LAYOUT_STD_SUB", "LAYOUT_STD_TRAIN"]
        if !stdLayouts.contains(layout) {
         let isBike = layout.contains("BIKE") || layout.contains("MOTO") || layout.contains("SCOOTER") || layout.contains("CHOPPER") || layout.contains("CYCLE")
         let targetLayout = isBike ? "LAYOUT_STD_MOTORBIKE" : "LAYOUT_STANDARD"
         let curNS=text as NSString
         if m.range.location != NSNotFound && m.range.location + m.range.length <= curNS.length {
          text=curNS.replacingCharacters(in:m.range,with:"<layout>\(targetLayout)</layout>")
          log("Vehicle compatibility: custom layout \(layout) -> \(targetLayout) in \(url.lastPathComponent)/\(e.path). Fix seat clipping.")
         }
        }
       }
      }

      // Pass 4: Normalize audioNameHash so vehicle horn and engine audio work 100% on mod vehicles
      ns=text as NSString
      for m in audioRE.matches(in:text,range:NSRange(location:0,length:ns.length)).reversed() {
       if m.numberOfRanges > 1 && m.range(at:1).location != NSNotFound {
        let audio=ns.substring(with:m.range(at:1)).trimmingCharacters(in:.whitespacesAndNewlines).uppercased()
        let stdAudios: Set<String> = ["ADDER", "ZENTORNO", "BATI", "SANCHEZ", "FAGGIO", "TAILGATER", "BALLER", "COMET2", "BUFFALO", "SULTAN", "ELEGY2", "FELON", "F620", "ORACLE", "AKUMA", "CARBONRS", "DAEMON", "DOUBLE"]
        if audio.isEmpty || !stdAudios.contains(audio) {
         let isBike = text.contains("LAYOUT_STD_MOTORBIKE") || text.contains("BIKE") || text.contains("MOTO")
         let targetAudio = isBike ? "BATI" : "ADDER"
         let curNS=text as NSString
         if m.range.location != NSNotFound && m.range.location + m.range.length <= curNS.length {
          text=curNS.replacingCharacters(in:m.range,with:"<audioNameHash>\(targetAudio)</audioNameHash>")
          log("Vehicle audio compatibility: custom audio \(audio) -> \(targetAudio) in \(url.lastPathComponent)/\(e.path). Fix horn & engine audio.")
         }
        }
       }
      }

      // Pass 5: Normalize sitInBike to true for motorbikes so the rider sits firmly on the saddle
      ns=text as NSString
      for m in sitRE.matches(in:text,range:NSRange(location:0,length:ns.length)).reversed() {
       let isBike = text.contains("LAYOUT_STD_MOTORBIKE") || text.contains("BIKE") || text.contains("MOTO")
       if isBike {
        let curNS=text as NSString
        if m.range.location != NSNotFound && m.range.location + m.range.length <= curNS.length {
         text=curNS.replacingCharacters(in:m.range,with:"<sitInBike value=\"true\" />")
        }
       }
      }

      if text != String(data:data,encoding:.utf8) {
       // Append an uncompressed binary entry; all geometry, nested RPFs and flags stay byte-identical.
       let count=Int(VehicleInstaller.word(out,4)),nsStart=16+16*count
       let nameSize=Int(VehicleInstaller.word(out,8))
       var entryAt:Int?=nil
       for i in 0..<count {
        let at=16+i*16,a=VehicleInstaller.word(out,at),c=VehicleInstaller.word(out,at+4)
        if c==0x7fffff00{continue}
        let offset=Int((UInt64(a)|UInt64(c)<<32)>>40 & 0xffffff)*512
        if offset==e.offset {entryAt=at;break}
       }
       guard nsStart+nameSize<=out.count,let at=entryAt else{throw ModError("Không tìm thấy entry metadata.")}
       while out.count%512 != 0{out.append(0)}
       let block=out.count/512,new=Data(text.utf8)
       func put(_ at:Int,_ n:UInt32){for i in 0..<4{out[at+i]=UInt8(truncatingIfNeeded:n>>(i*8))}}
       put(at,VehicleInstaller.word(out,at)&0xffff);put(at+4,UInt32(block)<<8);put(at+8,UInt32(new.count));put(at+12,0)
       out.append(new);changed=true
      }
     }
     if changed {
      let backup=url.deletingLastPathComponent().appendingPathComponent(".dlc.rpf.mulone-original")
      if !fm.fileExists(atPath:backup.path){try fm.copyItem(at:url,to:backup)}
      try out.write(to:url,options:.atomic)
     }
    }catch{log("Vehicle compatibility: \(error.localizedDescription); original pack retained on write failure.")}
   }
   for mod in mods {
    do{try JSONSerialization.data(withJSONObject:Array(names[mod.url] ?? []).sorted()).write(to:mod.url.appendingPathComponent(".mulone-vehicles.json"),options:.atomic)}catch{log("Vehicle list: \(error.localizedDescription)")}
   }
  }catch{log("Vehicle preparation failed: \(error.localizedDescription)")}
 }
 /// Build a pre-boot index for RPF resources whose size uses OpenIV's 32-bit
 /// extended header. The pinned engine otherwise masks their length to24bits.
 static func prepareLargeResources(log:(String)->Void) {
  let fm=FileManager.default
  var index:[String:[String:UInt64]]=[:]
  do {
   for mod in ModInstaller.installed().filter({$0.enabled}) {
    guard let walk=fm.enumerator(at:mod.url,includingPropertiesForKeys:nil,options:.skipsHiddenFiles) else{continue}
    for case let url as URL in walk where url.lastPathComponent.lowercased()=="dlc.rpf" {
     let archive=try RPFArchive(url:url)
     let setup=archive.entries.first{$0.path=="setup2.xml"}
     let xml=try setup.map{try archive.metadata($0)}.flatMap{String(data:$0,encoding:.utf8)} ?? ""
     let re=try NSRegularExpression(pattern:"<deviceName>\\s*([^<]+)\\s*</deviceName>",options:.caseInsensitive)
     let ns=xml as NSString
     let device=re.firstMatch(in:xml,range:NSRange(location:0,length:ns.length)).map{ns.substring(with:$0.range(at:1)).lowercased()}
     let pack=url.deletingLastPathComponent().lastPathComponent.lowercased()
     func scan(_ a:RPFArchive,_ label:String,_ depth:Int)throws {
      guard depth<8 else{throw ModError("DLC lồng quá sâu.")}
      for e in a.entries {
       if e.path.hasSuffix(".rpf"),let device {
        try scan(a.nested(e),device+":/"+e.path,depth+1)
       } else if e.resourceVersion != nil && e.size>=0xffffff {
        guard e.size<=256*1024*1024 else{throw ModError("Resource vượt256MiB: \(e.path)")}
        // Verify a complete compressed payload once per scan. Keep bytes intact.
        _ = try ModResource.normalize(a.resourceData(e),name:e.path)
        let key=label.lowercased()+"|"+e.path.lowercased()
        let item:[String:UInt64]=["size":UInt64(e.size),"system":UInt64(e.unpacked),"graphics":UInt64(e.graphicsFlags)]
        if let old=index[key],old != item{throw ModError("Hai DLC dùng chung đường dẫn resource lớn: \(key)")}
        index[key]=item
       }
      }
     }
     try scan(archive,"x64/dlcpacks/"+pack+"/dlc.rpf",0)
    }
   }
   try fm.createDirectory(at:ModInstaller.modsFolder,withIntermediateDirectories:true)
   try JSONSerialization.data(withJSONObject:index,options:.sortedKeys).write(to:ModInstaller.modsFolder.appendingPathComponent(".newgen-resource32.json"),options:.atomic)
   log("Large mod resources: \(index.count) verified extended-size entries indexed before boot.")
  }catch {
   // A stale index must never be used for changed archives.
   try? Data("{}".utf8).write(to:ModInstaller.modsFolder.appendingPathComponent(".newgen-resource32.json"),options:.atomic)
   log("Large resource validation failed: \(error.localizedDescription). Extended-size resources are not enabled.")
  }
 }

}
