#include <QApplication>
#include <QMainWindow>
int main(int argc, char* argv[]){
    QApplication app(argc, argv);
    QMainWindow window;
    window.setWindowTitle("Test");
    window.resize(1920,1080);
    window.show();

    return app.exec();
}
