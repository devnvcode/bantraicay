import Foundation

/// Boot-only preferences. Automatic preserves the regeneration baseline.
enum StreamingSettings {
    static var distanceStreaming: Bool {
        get { UserDefaults.standard.object(forKey: "distanceStreaming") as? Bool ?? true }
        set { UserDefaults.standard.set(newValue, forKey: "distanceStreaming") }
    }
    static let reserveValues = [-1, 0, 128, 192, 256]
    static var reserveProfileMB: Int {
        get { let value=UserDefaults.standard.object(forKey:"distanceReserveMB") as? Int ?? -1;return reserveValues.contains(value) ? value : -1 }
        set { UserDefaults.standard.set(reserveValues.contains(newValue) ? newValue : -1,forKey:"distanceReserveMB") }
    }
    static func automaticReserveMB(physicalBytes: UInt64) -> Int {
        let mb=physicalBytes/1_048_576
        return mb<=4608 ? 128 : (mb<7168 ? 192 : 256)
    }
    static var selectedReserveMB: Int { reserveProfileMB<0 ? automaticReserveMB(physicalBytes:ProcessInfo.processInfo.physicalMemory) : reserveProfileMB }
    static let reclaimDistances=[40,60,80,120,160]
    static var reclaimDistance: Int {
        get {let value=UserDefaults.standard.object(forKey:"distanceReclaimMeters") as? Int ?? 80;return reclaimDistances.contains(value) ? value : 80}
        set {UserDefaults.standard.set(reclaimDistances.contains(newValue) ? newValue : 80,forKey:"distanceReclaimMeters")}
    }
    static let distanceValues = ["0", "0.2", "0.4", "0.6", "0.8", "1"]
    static let textureValues = [384, 512, 640, 768, 1024, 1280]
    static var distanceScale: String? {
        get {
            guard let value = UserDefaults.standard.string(forKey: "RegenerationExtendedDistance"),
                  distanceValues.contains(value) else { return nil }
            return value
        }
        set {
            UserDefaults.standard.set(newValue.flatMap { distanceValues.contains($0) ? $0 : nil }, forKey: "RegenerationExtendedDistance")
        }
    }
    static var textureBudgetMB: Int {
        get {
            let value = UserDefaults.standard.integer(forKey: "RegenerationTextureBudget")
            return textureValues.contains(value) ? value : 0
        }
        set { UserDefaults.standard.set(textureValues.contains(newValue) ? newValue : 0, forKey: "RegenerationTextureBudget") }
    }
    static func applyTextureBudget() {
        setenv("PG_DISTANCE_STREAMING", distanceStreaming ? "1" : "0", 1)
        setenv("PG_DISTANCE_RESERVE_MB", String(selectedReserveMB), 1)
        setenv("PG_DISTANCE_METERS", String(reclaimDistance), 1)
        if textureBudgetMB == 0 { unsetenv("PG_VIDEO_MEMORY_MB") }
        else { setenv("PG_VIDEO_MEMORY_MB", String(textureBudgetMB), 1) }
    }
}
