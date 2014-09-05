
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <memory>

#include <QMainWindow>

class IPluginLoader;
class IProject;
class IProjectManager;

class CentralWidget;

class MainWindow final: public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        MainWindow(const MainWindow &) = delete;
        virtual ~MainWindow();

        MainWindow operator=(const MainWindow &) = delete;

        void set(IProjectManager *);
        void set(IPluginLoader *);

    private:
        IProjectManager*          m_prjManager;
        IPluginLoader*            m_pluginLoader;
        std::shared_ptr<IProject> m_currentPrj;
        CentralWidget*            m_centralWidget;

        void closeEvent(QCloseEvent *);

    private slots:
        void newProject();
        void openProject();
};

#endif // MAINWINDOW_HPP
