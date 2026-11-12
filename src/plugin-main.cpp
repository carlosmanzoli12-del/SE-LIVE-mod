#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>

#include <QApplication>
#include <QStyle>
#include <QComboBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPointer>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("streamforge", "en-US")

namespace {
constexpr const char *kDockId = "streamforge.main.dock";
QPointer<QWidget> g_dock;

QString obsString(const char *value)
{
    return value ? QString::fromUtf8(value) : QString();
}

struct SceneCollector {
    QListWidget *list = nullptr;
};

bool collectSceneItem(obs_scene_t *, obs_sceneitem_t *item, void *opaque)
{
    auto *collector = static_cast<SceneCollector *>(opaque);
    obs_source_t *source = obs_sceneitem_get_source(item);
    if (!source)
        return true;
    const char *name = obs_source_get_name(source);
    auto *row = new QListWidgetItem(obsString(name));
    row->setData(Qt::UserRole, obs_sceneitem_visible(item));
    row->setIcon(QApplication::style()->standardIcon(
        obs_sceneitem_visible(item) ? QStyle::SP_DialogApplyButton : QStyle::SP_DialogCancelButton));
    collector->list->addItem(row);
    return true;
}

class StreamForgeDock final : public QWidget {
public:
    StreamForgeDock()
    {
        setObjectName(QStringLiteral("StreamForgeDock"));
        setMinimumSize(430, 560);
        setStyleSheet(R"(
            QWidget { background:#11151d; color:#e8edf5; font-family:'Segoe UI'; font-size:10pt; }
            QGroupBox { border:1px solid #2b3443; border-radius:9px; margin-top:12px; padding:12px; font-weight:600; }
            QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 5px; color:#9db2ce; }
            QPushButton { background:#263449; border:1px solid #354761; border-radius:6px; padding:8px 12px; }
            QPushButton:hover { background:#344967; }
            QPushButton:disabled { color:#748094; background:#1a202b; }
            QPushButton#primary { background:#3869d4; border-color:#4779e7; font-weight:600; }
            QPushButton#danger { background:#7c3440; border-color:#a34857; }
            QListWidget, QTableWidget, QLineEdit, QComboBox { background:#171e29; border:1px solid #303b4d; border-radius:5px; padding:5px; }
            QTabWidget::pane { border:1px solid #2b3443; border-radius:7px; }
            QTabBar::tab { background:#1a202b; padding:8px 12px; margin-right:3px; }
            QTabBar::tab:selected { background:#2a3c56; color:#ffffff; }
            QLabel#muted { color:#91a0b5; }
            QLabel#headline { font-size:18pt; font-weight:700; color:#ffffff; }
            QLabel#status { color:#9eb6d5; }
        )");

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(14, 14, 14, 14);
        root->setSpacing(10);

        auto *brand = new QLabel(QStringLiteral("STREAMFORGE"));
        brand->setObjectName("headline");
        root->addWidget(brand);
        auto *tagline = new QLabel(QStringLiteral("Creator control center · OBS Studio 30.0.2"));
        tagline->setObjectName("muted");
        root->addWidget(tagline);

        tabs_ = new QTabWidget(this);
        root->addWidget(tabs_, 1);
        buildDashboard();
        buildScenes();
        buildDestinations();
        buildChat();
        buildOverlays();
        buildActivity();
        buildSettings();

        refreshTimer_ = new QTimer(this);
        connect(refreshTimer_, &QTimer::timeout, this, [this] { refreshAll(); });
        refreshTimer_->start(1500);
        refreshAll();
    }

private:
    QTabWidget *tabs_ = nullptr;
    QLabel *streamState_ = nullptr;
    QLabel *recordState_ = nullptr;
    QPushButton *streamButton_ = nullptr;
    QPushButton *recordButton_ = nullptr;
    QListWidget *sceneList_ = nullptr;
    QListWidget *sourceList_ = nullptr;
    QTableWidget *destinationTable_ = nullptr;
    QListWidget *activityList_ = nullptr;
    QLineEdit *overlayUrl_ = nullptr;
    QTimer *refreshTimer_ = nullptr;

    void buildDashboard()
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        auto *status = new QGroupBox(QStringLiteral("LIVE STATUS"));
        auto *statusLayout = new QFormLayout(status);
        streamState_ = new QLabel("Offline");
        recordState_ = new QLabel("Not recording");
        statusLayout->addRow("Broadcast", streamState_);
        statusLayout->addRow("Recording", recordState_);
        layout->addWidget(status);

        auto *actions = new QGroupBox(QStringLiteral("QUICK CONTROLS"));
        auto *actionsLayout = new QHBoxLayout(actions);
        streamButton_ = new QPushButton("Start stream");
        streamButton_->setObjectName("primary");
        recordButton_ = new QPushButton("Start recording");
        actionsLayout->addWidget(streamButton_);
        actionsLayout->addWidget(recordButton_);
        layout->addWidget(actions);

        auto *replay = new QPushButton("Save replay buffer");
        connect(replay, &QPushButton::clicked, this, [] {
            if (obs_frontend_replay_buffer_active())
                obs_frontend_replay_buffer_save();
            else
                QMessageBox::information(nullptr, "StreamForge", "Replay Buffer is not active in OBS.");
        });
        layout->addWidget(replay);
        connect(streamButton_, &QPushButton::clicked, this, [this] {
            if (obs_frontend_streaming_active()) obs_frontend_streaming_stop();
            else obs_frontend_streaming_start();
            refreshStatus();
        });
        connect(recordButton_, &QPushButton::clicked, this, [this] {
            if (obs_frontend_recording_active()) obs_frontend_recording_stop();
            else obs_frontend_recording_start();
            refreshStatus();
        });
        layout->addStretch();
        tabs_->addTab(page, "Overview");
    }

    void buildScenes()
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        auto *sceneBox = new QGroupBox("SCENE COLLECTION");
        auto *sceneLayout = new QVBoxLayout(sceneBox);
        sceneList_ = new QListWidget;
        sceneLayout->addWidget(sceneList_);
        auto *refresh = new QPushButton("Refresh scenes");
        sceneLayout->addWidget(refresh);
        layout->addWidget(sceneBox, 1);

        auto *sourceBox = new QGroupBox("SOURCES IN SELECTED SCENE");
        auto *sourceLayout = new QVBoxLayout(sourceBox);
        sourceList_ = new QListWidget;
        sourceLayout->addWidget(sourceList_);
        auto *hint = new QLabel("Double-click a source to show or hide it.");
        hint->setObjectName("muted");
        sourceLayout->addWidget(hint);
        layout->addWidget(sourceBox, 1);

        connect(refresh, &QPushButton::clicked, this, [this] { refreshScenes(); });
        connect(sceneList_, &QListWidget::currentRowChanged, this, [this](int row) {
            if (row < 0) return;
            const QString name = sceneList_->item(row)->text();
            obs_frontend_source_list scenes = {};
            obs_frontend_get_scenes(&scenes);
            for (size_t i = 0; i < scenes.sources.num; ++i) {
                obs_source_t *scene = scenes.sources.array[i];
                if (name == obsString(obs_source_get_name(scene))) {
                    obs_frontend_set_current_scene(scene);
                    break;
                }
            }
            obs_frontend_source_list_free(&scenes);
            refreshSources();
        });
        connect(sourceList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *row) {
            const QString wanted = row->text();
            obs_source_t *current = obs_frontend_get_current_scene();
            obs_scene_t *scene = current ? obs_scene_from_source(current) : nullptr;
            if (scene) {
                struct ToggleData { QString name; bool done = false; } data{wanted, false};
                obs_scene_enum_items(scene, [](obs_scene_t *, obs_sceneitem_t *item, void *opaque) {
                    auto *d = static_cast<ToggleData *>(opaque);
                    obs_source_t *src = obs_sceneitem_get_source(item);
                    if (src && d->name == obsString(obs_source_get_name(src))) {
                        obs_sceneitem_set_visible(item, !obs_sceneitem_visible(item));
                        d->done = true;
                        return false;
                    }
                    return true;
                }, &data);
            }
            if (current) obs_source_release(current);
            refreshSources();
        });
        layout->addStretch();
        tabs_->addTab(page, "Scenes");
    }

    void buildDestinations()
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        auto *note = new QLabel("Plan and organize your streaming destinations. Multi-output transport is an integration milestone; this panel does not start independent RTMP outputs yet.");
        note->setWordWrap(true);
        note->setObjectName("muted");
        layout->addWidget(note);
        destinationTable_ = new QTableWidget(0, 3);
        destinationTable_->setHorizontalHeaderLabels({"Platform", "Server / endpoint", "State"});
        destinationTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        destinationTable_->verticalHeader()->hide();
        layout->addWidget(destinationTable_);
        auto *add = new QPushButton("Add destination");
        connect(add, &QPushButton::clicked, this, [this] {
            bool ok = false;
            const QString platform = QInputDialog::getText(this, "New destination", "Platform / destination name:", QLineEdit::Normal, {}, &ok);
            if (!ok || platform.trimmed().isEmpty()) return;
            const QString endpoint = QInputDialog::getText(this, "Destination endpoint", "RTMP(S) server URL (optional):", QLineEdit::Normal, {}, &ok);
            if (!ok) return;
            const int row = destinationTable_->rowCount();
            destinationTable_->insertRow(row);
            destinationTable_->setItem(row, 0, new QTableWidgetItem(platform.trimmed()));
            destinationTable_->setItem(row, 1, new QTableWidgetItem(endpoint.trimmed()));
            destinationTable_->setItem(row, 2, new QTableWidgetItem("Planned"));
        });
        layout->addWidget(add);
        tabs_->addTab(page, "Destinations");
    }

    void buildChat()
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        auto *title = new QLabel("UNIFIED CHAT");
        title->setStyleSheet("font-weight:700; color:#9db2ce;");
        layout->addWidget(title);
        auto *description = new QLabel("The unified chat workspace is reserved for platform adapters. Connectors require each platform's approved OAuth flow and API permissions; no credentials are requested or transmitted by this starter build.");
        description->setWordWrap(true);
        layout->addWidget(description);
        auto *twitch = new QPushButton("Open Twitch chat in browser");
        auto *youtube = new QPushButton("Open YouTube live chat help");
        connect(twitch, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl("https://www.twitch.tv/popout")); });
        connect(youtube, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl("https://support.google.com/youtube/answer/252?hl=pt-BR")); });
        layout->addWidget(twitch);
        layout->addWidget(youtube);
        auto *status = new QLabel("No chat providers connected.");
        status->setObjectName("muted");
        layout->addWidget(status);
        layout->addStretch();
        tabs_->addTab(page, "Unified Chat");
    }

    void buildOverlays()
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        auto *box = new QGroupBox("ADD A WEB OVERLAY TO THE CURRENT SCENE");
        auto *form = new QFormLayout(box);
        overlayUrl_ = new QLineEdit;
        overlayUrl_->setPlaceholderText("https://example.com/overlay");
        form->addRow("Overlay URL", overlayUrl_);
        auto *add = new QPushButton("Add browser overlay");
        add->setObjectName("primary");
        form->addRow(add);
        connect(add, &QPushButton::clicked, this, [this] {
            const QUrl url(overlayUrl_->text().trimmed());
            if (!url.isValid() || (url.scheme() != "https" && url.scheme() != "http")) {
                QMessageBox::warning(this, "StreamForge", "Enter a valid HTTP or HTTPS URL.");
                return;
            }
            obs_source_t *current = obs_frontend_get_current_scene();
            obs_scene_t *scene = current ? obs_scene_from_source(current) : nullptr;
            if (!scene) {
                if (current) obs_source_release(current);
                QMessageBox::warning(this, "StreamForge", "Select a scene first.");
                return;
            }
            obs_data_t *settings = obs_data_create();
            obs_data_set_string(settings, "url", url.toString().toUtf8().constData());
            obs_data_set_int(settings, "width", 1920);
            obs_data_set_int(settings, "height", 1080);
            static int overlayIndex = 0;
            const QString sourceName = QString("StreamForge Overlay %1").arg(++overlayIndex);
            obs_source_t *browser = obs_source_create("browser_source", sourceName.toUtf8().constData(), settings, nullptr);
            obs_data_release(settings);
            if (browser) {
                obs_scene_add(scene, browser);
                obs_source_release(browser);
                QMessageBox::information(this, "StreamForge", "Browser overlay added to the current scene.");
            } else {
                QMessageBox::warning(this, "StreamForge", "OBS Browser Source is unavailable. Make sure obs-browser is installed and enabled.");
            }
            obs_source_release(current);
        });
        layout->addWidget(box);
        auto *note = new QLabel("This creates a native OBS Browser Source at 1920×1080. Edit its transform and dimensions using OBS's standard controls.");
        note->setWordWrap(true);
        note->setObjectName("muted");
        layout->addWidget(note);
        layout->addStretch();
        tabs_->addTab(page, "Overlays");
    }

    void buildActivity()
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        activityList_ = new QListWidget;
        activityList_->addItem("StreamForge loaded and ready.");
        activityList_->addItem("OBS events will appear here during this session.");
        layout->addWidget(activityList_);
        tabs_->addTab(page, "Activity");
    }

    void buildSettings()
    {
        auto *page = new QWidget;
        auto *layout = new QVBoxLayout(page);
        layout->addWidget(new QLabel("STREAMFORGE SETTINGS"));
        auto *info = new QLabel("Version 0.1.0\nTarget host: OBS Studio 30.0.2\nBrand: StreamForge\n\nNo API keys or stream keys are stored by this build. Platform authentication and multi-output transport are separate development milestones.");
        info->setWordWrap(true);
        layout->addWidget(info);
        layout->addStretch();
        tabs_->addTab(page, "Settings");
    }

    void refreshStatus()
    {
        const bool streaming = obs_frontend_streaming_active();
        const bool recording = obs_frontend_recording_active();
        streamState_->setText(streaming ? "LIVE" : "Offline");
        recordState_->setText(recording ? "Recording" : "Not recording");
        streamButton_->setText(streaming ? "Stop stream" : "Start stream");
        streamButton_->setObjectName(streaming ? "danger" : "primary");
        streamButton_->style()->unpolish(streamButton_);
        streamButton_->style()->polish(streamButton_);
        recordButton_->setText(recording ? "Stop recording" : "Start recording");
    }

    void refreshScenes()
    {
        const QString previous = sceneList_->currentItem() ? sceneList_->currentItem()->text() : QString();
        sceneList_->clear();
        obs_frontend_source_list scenes = {};
        obs_frontend_get_scenes(&scenes);
        QString currentName;
        obs_source_t *current = obs_frontend_get_current_scene();
        if (current) currentName = obsString(obs_source_get_name(current));
        for (size_t i = 0; i < scenes.sources.num; ++i) {
            const QString name = obsString(obs_source_get_name(scenes.sources.array[i]));
            sceneList_->addItem(name);
            if (name == currentName || name == previous)
                sceneList_->setCurrentRow(sceneList_->count() - 1);
        }
        if (current) obs_source_release(current);
        obs_frontend_source_list_free(&scenes);
        refreshSources();
    }

    void refreshSources()
    {
        sourceList_->clear();
        obs_source_t *current = obs_frontend_get_current_scene();
        obs_scene_t *scene = current ? obs_scene_from_source(current) : nullptr;
        if (scene) {
            SceneCollector collector{sourceList_};
            obs_scene_enum_items(scene, collectSceneItem, &collector);
        }
        if (current) obs_source_release(current);
    }

    void refreshAll()
    {
        refreshStatus();
        if (sceneList_->count() == 0) refreshScenes();
    }
};

void openDock(void *)
{
    if (g_dock)
        g_dock->show();
}

void frontendEvent(enum obs_frontend_event event, void *)
{
    if (!g_dock) return;
    if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED || event == OBS_FRONTEND_EVENT_STREAMING_STOPPED ||
        event == OBS_FRONTEND_EVENT_RECORDING_STARTED || event == OBS_FRONTEND_EVENT_RECORDING_STOPPED ||
        event == OBS_FRONTEND_EVENT_SCENE_CHANGED) {
        auto *dock = static_cast<StreamForgeDock *>(g_dock.data());
        if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED) dock->findChild<QTabWidget *>()->setCurrentIndex(1);
    }
}
} // namespace

bool obs_module_load(void)
{
    blog(LOG_INFO, "StreamForge 0.1.0 loading for OBS Studio 30.0.2+");
    auto *dock = new StreamForgeDock;
    g_dock = dock;
    if (!obs_frontend_add_dock_by_id(kDockId, "StreamForge", dock)) {
        delete dock;
        g_dock = nullptr;
        blog(LOG_ERROR, "StreamForge could not register its OBS dock");
        return false;
    }
    obs_frontend_add_tools_menu_item("Open StreamForge", openDock, nullptr);
    obs_frontend_add_event_callback(frontendEvent, nullptr);
    return true;
}

void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(frontendEvent, nullptr);
    obs_frontend_remove_dock(kDockId);
    g_dock = nullptr;
    blog(LOG_INFO, "StreamForge unloaded");
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Independent creator control center for OBS Studio";
}
