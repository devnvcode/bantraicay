import UIKit

/// The first Settings page: one row per category, each opening its own page (a SettingsViewController
/// showing only that category's sections). Categories that do nothing right now are left out: game files
/// and the game mode during play, cheats before it.
final class SettingsHomeViewController: UITableViewController {
    private let engineRunning: Bool
    private let runningMode: GameMode?
    var resolutionChanged: (() -> Void)?
    var controlsChanged: (() -> Void)?
    var editLayout: (() -> Void)?
    var targetingChanged: (() -> Void)?
    var gameModeChanged: (() -> Void)?
    var cheatSelected: ((StoryCheat) -> Void)?
    var moneyEntered: ((Int32) -> Void)?
    var vehicleSelected: ((SpawnVehicle, Bool) -> Void)?
    var nativeCheatSelected: ((NativeCheatAction) -> Void)?

    private struct Category {
        let icon: String
        let title: String
        let summary: () -> String
        let sections: Set<SettingsViewController.Section>
        var page: (() -> UIViewController)? = nil
    }
    private var categories: [Category] {
        let settings = GameSettings.shared, running = engineRunning
        var list = [
            Category(icon: "display", title: AppLanguage.choose("Graphics"), summary: {
                AppLanguage.interpolate(AppLanguage.choose("{arg0}, resolution, frame rate, memory"), [settings.graphicsPreset.title])
            }, sections: [.graphics]),
            Category(icon: "gamecontroller", title: AppLanguage.choose("Controls"), summary: {
                AppLanguage.choose("Camera, buttons, layout, gyro, controller and keyboard")
            }, sections: [.controls, .gyro]),
        ]
        if engineRunning {
            list.append(Category(icon: "car.fill", title: "Gọi xe mod", summary: { "Chỉ hiện những xe mod đang bật" }, sections: [], page: { [weak self] in
                let page = VehicleSpawnViewController()
                page.vehicleSelected = self?.vehicleSelected
                return page
            }))
            list.append(Category(icon: "wand.and.stars", title: AppLanguage.choose("Cheats"), summary: {
                AppLanguage.choose("Vehicles, repairs, weather, time, money and more")
            }, sections: [.cheats]))
        }
        list.append(Category(icon: "globe", title: engineRunning ? AppLanguage.choose("Language") : AppLanguage.choose("Game mode and language"), summary: {
            running ? AppLanguage.current.name : "\(settings.gameMode.title) · \(AppLanguage.current.name)"
        }, sections: [.language, .mode]))
        if !engineRunning {
            list.append(Category(icon: "viewfinder", title: "Tầm nhìn và texture (4 GB)", summary: { "Extended Distance Scaling · Extended Texture Budget" }, sections: [], page: { StreamingSettingsViewController() }))
            list.append(Category(icon: "folder", title: AppLanguage.choose("Game files"), summary: {
                AppLanguage.choose("Where the game is, checking and downloading its files")
            }, sections: [.files]))
        }
        list.append(Category(icon: "shippingbox", title: AppLanguage.choose("Mods"), summary: {
            let count = ModInstaller.installed().filter(\.enabled).count
            return count == 0 ? AppLanguage.choose("Install car, skin and script mods") : AppLanguage.interpolate(AppLanguage.choose("{arg0} mods on"), [String(count)])
        }, sections: [], page: { ModsViewController() }))
        list.append(Category(icon:"puzzlepiece.extension",title:"Plugin Menyoo",summary:{"Bật/tắt Menyoo tích hợp, hướng dẫn nút điều khiển"},sections:[],page:{BundledMenyooViewController()}))
        list.append(Category(icon:"flame",title:"Turbo và lửa bô (thử nghiệm)",summary:{"Turbo, boost theo số, anti-lag, âm thanh và đồng hồ"},sections:[],page:{TurboSettingsViewController()}))
        list.append(Category(icon:"memorychip",title:"RAM và quyền bộ nhớ",summary:{"Kiểm tra quyền tăng RAM, bỏ cap riêng và xuất entitlement"},sections:[],page:{MemoryAccessViewController(engineRunning:running)}))
        list.append(Category(icon: "iphone", title: AppLanguage.choose("This device"), summary: {
            AppLanguage.choose("Memory, logs and the Telegram group")
        }, sections: [.device]))
        return list
    }

    init(engineRunning: Bool, runningMode: GameMode?) {
        self.engineRunning = engineRunning
        self.runningMode = runningMode
        super.init(style: .insetGrouped)
        title = AppLanguage.choose("Settings")
        modalPresentationStyle = .formSheet
        overrideUserInterfaceStyle = .dark
    }
    required init?(coder: NSCoder) { fatalError("init(coder:) has not been implemented") }

    override func viewDidLoad() {
        super.viewDidLoad()
        navigationItem.rightBarButtonItem = UIBarButtonItem(title: AppLanguage.choose("Done"), primaryAction: UIAction { [weak self] _ in self?.dismiss(animated: true) })
        navigationItem.rightBarButtonItem?.tintColor = Theme.accent
        navigationItem.backButtonDisplayMode = .minimal
        Theme.styleSettingsTable(tableView)
        // Automation: --settings-section opens that category's page.
        let arguments = ProcessInfo.processInfo.arguments
        if let i = arguments.firstIndex(of: "--settings-section"), i + 1 < arguments.count {
            let wanted: SettingsViewController.Section? = ["files": .files, "graphics": .graphics, "resolution": .graphics, "device": .device, "controls": .controls, "cheats": .cheats, "spawn": .cheats][arguments[i + 1]]
            if let wanted, let index = categories.firstIndex(where: { $0.sections.contains(wanted) }) {
                DispatchQueue.main.async { self.open(self.categories[index], animated: false) }
            }
        }
    }

    // Titles can change with the language and the summaries with the settings.
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        title = AppLanguage.choose("Settings")
        tableView.reloadData()
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int { categories.count }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let category = categories[indexPath.row]
        let cell = UITableViewCell(style: .subtitle, reuseIdentifier: nil)
        var content = cell.defaultContentConfiguration()
        content.image = UIImage(systemName: category.icon)
        content.imageProperties.tintColor = Theme.accent
        content.text = category.title
        content.textProperties.font = Theme.body(17, .semibold)
        content.textProperties.color = Theme.text
        content.secondaryText = category.summary()
        content.secondaryTextProperties.color = Theme.secondaryText
        cell.contentConfiguration = content
        cell.accessoryType = .disclosureIndicator
        cell.backgroundColor = Theme.rowBackground
        let selected = UIView()
        selected.backgroundColor = Theme.rowSelected
        cell.selectedBackgroundView = selected
        return cell
    }

    override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
        "Mugoat NewGen" + " \(Theme.appVersion)"
    }

    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        open(categories[indexPath.row], animated: true)
    }

    private func open(_ category: Category, animated: Bool) {
        if let page = category.page { navigationController?.pushViewController(page(), animated: animated); return }
        let page = SettingsViewController(engineRunning: engineRunning, runningMode: runningMode, title: category.title, sections: category.sections)
        page.resolutionChanged = resolutionChanged
        page.controlsChanged = controlsChanged
        page.editLayout = editLayout
        page.targetingChanged = targetingChanged
        page.gameModeChanged = gameModeChanged
        page.cheatSelected = cheatSelected
        page.moneyEntered = moneyEntered
        page.nativeCheatSelected = nativeCheatSelected
        page.vehicleSelected = vehicleSelected
        navigationController?.pushViewController(page, animated: animated)
    }
}
