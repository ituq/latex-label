#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QSettings>
#include <iostream>
#include "LatexLabel.h"
#include <QScrollArea>

//Simple text streaming
QTimer* stream_timer = nullptr;
QStringList words;
int word_index = 0;
LatexLabel* label_ref = nullptr;

void add_next_word() {
    if (!label_ref || word_index >= words.size()) {
        stream_timer->stop();
        return;
    }

    QString word = words[word_index] + " ";
    label_ref->appendText(word);
    word_index++;
}

void start_text_streaming(const QString& content, LatexLabel* label) {
    label_ref = label;
    word_index = 0;

    label->setText("");
    words = content.split(' ', Qt::SkipEmptyParts);

    if (!stream_timer) {
        stream_timer = new QTimer();
        QObject::connect(stream_timer, &QTimer::timeout, add_next_word);
    }

    stream_timer->start(50); //Much faster - 20ms between words
}


int main(int argc, char* argv[]){
    QApplication app(argc, argv);
    QMainWindow window;

    // Disable debug mode to remove bounding boxes
    tex::LaTeX::setDebug(false);

    // Get the absolute path to the resources directory
    QString appDir = QCoreApplication::applicationDirPath();
    QString resPath = appDir + "/res";
    std::cout << "Resource path: " << resPath.toStdString() << std::endl;

    // Initialize MicroTeX with the absolute path to resources
    tex::LaTeX::init(resPath.toStdString());

    window.setWindowTitle("LaTeX Label Test Suite");

    // Initialize settings
    QSettings settings("LatexLabel", "TestSuite");

    // Create main widget and layout
    QWidget* centralWidget = new QWidget(&window);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Create test file selector
    QHBoxLayout* controlLayout = new QHBoxLayout();
    QLabel* selectorLabel = new QLabel("Test File:");
    QComboBox* fileSelector = new QComboBox();
    QPushButton* loadButton = new QPushButton("Load Test");
    QPushButton* browseButton = new QPushButton("Browse...");

    controlLayout->addWidget(selectorLabel);
    controlLayout->addWidget(fileSelector);
    controlLayout->addWidget(loadButton);
    controlLayout->addWidget(browseButton);
    controlLayout->addStretch();

    mainLayout->addLayout(controlLayout);


    QScrollArea* scroll = new QScrollArea;

    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scroll->setWidgetResizable(true);
    LatexLabel* label = new LatexLabel(scroll);
    label->setTextSize(20);
    label->setText("Select a test file to load markdown content with LaTeX support.");
    scroll->setWidget(label);

    mainLayout->addWidget(scroll);
    window.setCentralWidget(centralWidget);

    // Populate test files from tests directory
    QString testsDir = QCoreApplication::applicationDirPath() + "/tests";
    QDir testDirectory(testsDir);

    // If tests directory doesn't exist, try relative path
    if (!testDirectory.exists()) {
        testsDir = "../tests";
        testDirectory.setPath(testsDir);
    }

    if (testDirectory.exists()) {
        QStringList filters;
        filters << "*.md" << "*.markdown";
        testDirectory.setNameFilters(filters);
        QStringList testFiles = testDirectory.entryList(QDir::Files);

        for (const QString& file : testFiles) {
            fileSelector->addItem(file);
        }

        if (testFiles.isEmpty()) {
            fileSelector->addItem("No test files found");
            loadButton->setEnabled(false);
        }
    } else {
        fileSelector->addItem("Tests directory not found");
        loadButton->setEnabled(false);
        std::cout << "Tests directory not found at: " << testsDir.toStdString() << std::endl;
    }

        // Connect file selector to save selection when changed
    QObject::connect(fileSelector, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
        [&](const QString &text) {
            if (!text.isEmpty() && text != "No test files found" && text != "Tests directory not found") {
                settings.setValue("lastSelectedFile", text);
            }
        });

    // Connect load button
    QObject::connect(loadButton, &QPushButton::clicked, [&]() {
        printf("============================================\n===========================NEW TEST===============================\n============================================");
        QString selectedFile = fileSelector->currentText();
        if (selectedFile.isEmpty() || selectedFile == "No test files found" || selectedFile == "Tests directory not found") {
            return;
        }

        QString filePath = testsDir + "/" + selectedFile;
        QFile file(filePath);

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            label->setText(content);
            //start_text_streaming(content, label);
            label->printSegmentsStructure();

            // Save the selected file to settings
            settings.setValue("lastSelectedFile", selectedFile);

            std::cout << "Loaded test file: " << selectedFile.toStdString() << std::endl;
        } else {
            QMessageBox::warning(&window, "Error", "Could not open file: " + filePath);
        }
    });

    // Connect browse button
    QObject::connect(browseButton, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&window,
            "Open Markdown Test File",
            testsDir,
            "Markdown Files (*.md *.markdown);;All Files (*)");

        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                file.close();

                start_text_streaming(content, label);

                // Add to selector if it's a new file
                QFileInfo fileInfo(fileName);
                QString baseName = fileInfo.fileName();
                if (fileSelector->findText(baseName) == -1) {
                    fileSelector->addItem(baseName);
                    fileSelector->setCurrentText(baseName);
                }

                // Save the selected file to settings
                settings.setValue("lastSelectedFile", baseName);

                std::cout << "Loaded file: " << fileName.toStdString() << std::endl;
            } else {
                QMessageBox::warning(&window, "Error", "Could not open file: " + fileName);
            }
        }
    });

    // Auto-load last selected test file or first available test file
    QString lastSelectedFile = settings.value("lastSelectedFile", "").toString();
    bool loadedLastSelected = false;

    // Try to load the last selected file first
    if (!lastSelectedFile.isEmpty()) {
        int index = fileSelector->findText(lastSelectedFile);
        if (index != -1) {
            fileSelector->setCurrentIndex(index);
            loadButton->click();
            loadedLastSelected = true;
            std::cout << "Auto-loaded last selected file: " << lastSelectedFile.toStdString() << std::endl;
        } else {
            std::cout << "Last selected file not found: " << lastSelectedFile.toStdString() << std::endl;
        }
    }

    // If we couldn't load the last selected file, load the first available one
    if (!loadedLastSelected && fileSelector->count() > 0 &&
        fileSelector->itemText(0) != "No test files found" &&
        fileSelector->itemText(0) != "Tests directory not found") {
        loadButton->click();
        std::cout << "Auto-loaded first available file: " << fileSelector->itemText(0).toStdString() << std::endl;
    }

    window.resize(800, 600);
    window.show();

    int retn = app.exec();

    // Clean up MicroTeX resources
    tex::LaTeX::release();
    return retn;
}
