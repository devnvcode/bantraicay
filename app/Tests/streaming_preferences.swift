import Foundation
@main struct StreamingPreferenceTest {
 static func main(){
  let defaults=UserDefaults.standard
  let keys=["distanceReserveMB","distanceReclaimMeters","distanceStreaming","RegenerationTextureBudget"]
  let previous=keys.map{defaults.object(forKey:$0)}
  defer{for (key,value) in zip(keys,previous){if let value=value{defaults.set(value,forKey:key)}else{defaults.removeObject(forKey:key)}}}
  keys.forEach{defaults.removeObject(forKey:$0)}
  assert(StreamingSettings.distanceStreaming && StreamingSettings.reserveProfileMB == -1 && StreamingSettings.reclaimDistance == 80)
  for (mb,expected) in [(2048,128),(3750,128),(4096,128),(4608,128),(5770,192),(6144,192),(7167,192),(7168,256),(7680,256),(8192,256),(16384,256)]{
   assert(StreamingSettings.automaticReserveMB(physicalBytes:UInt64(mb)*1_048_576)==expected)
  }
  for value in StreamingSettings.reserveValues{
   StreamingSettings.reserveProfileMB=value
   StreamingSettings.applyTextureBudget()
   assert(String(cString:getenv("PG_DISTANCE_RESERVE_MB"))==String(StreamingSettings.selectedReserveMB))
  }
  for value in StreamingSettings.reclaimDistances{
   StreamingSettings.reclaimDistance=value;StreamingSettings.applyTextureBudget()
   assert(String(cString:getenv("PG_DISTANCE_METERS"))==String(value))
  }
  StreamingSettings.reclaimDistance=99;assert(StreamingSettings.reclaimDistance==80)
  StreamingSettings.reserveProfileMB=999;assert(StreamingSettings.reserveProfileMB == -1)
  StreamingSettings.reserveProfileMB=0;assert(StreamingSettings.selectedReserveMB==0)
  StreamingSettings.distanceStreaming=false;StreamingSettings.applyTextureBudget();assert(String(cString:getenv("PG_DISTANCE_STREAMING"))=="0")
  print("PASS production Swift streaming settings: migration, all hardware/profile tiers, env, opt-out, invalid values")
 }
}
