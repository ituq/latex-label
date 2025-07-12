#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "LatexLabel.h"

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

    // Print the actual resource path being used
    std::cout << "MicroTeX resource path: " << tex::LaTeX::getResRootPath() << std::endl;

    window.setWindowTitle("LaTeX Test - Multiple Text Sizes");

    // Create a layout to show different text sizes
    QWidget* centralWidget = new QWidget(&window);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    // Small text (10pt)
    LatexLabel* smallLabel = new LatexLabel(centralWidget);
    smallLabel->setTextSize(10);
    smallLabel->setText("Small text (10pt): The equation $E = mc^2$ and sum $\\sum_{i=1}^{n} x_i$ are inline.");
    layout->addWidget(smallLabel);

    // Medium text (14pt)
    LatexLabel* mediumLabel = new LatexLabel(centralWidget);
    mediumLabel->setTextSize(14);
    mediumLabel->setText("Medium text (14pt): Here's $\\alpha + \\beta = \\gamma$ and fraction $\\frac{1}{2}$.");
    layout->addWidget(mediumLabel);

    // Large text (18pt)
    LatexLabel* largeLabel = new LatexLabel(centralWidget);
    largeLabel->setTextSize(18);
    largeLabel->setText("Large text (18pt): Mathematical expressions like $\\int_{0}^{1} x dx$ scale nicely!Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    layout->addWidget(largeLabel);

    // Test streaming append functionality - words added every 200ms
    LatexLabel* streamLabel = new LatexLabel(centralWidget);
    streamLabel->setTextSize(14);
    streamLabel->setText("Streaming test: ");
    layout->addWidget(streamLabel);

    // Text to stream word by word - deliberately split LaTeX expressions to test incomplete handling
    QStringList* streamWords = new QStringList({
        "This", "shows", "incremental", "parsing", "with", "math",
        "$\\alpha", "+", "\\beta", "=", "\\gamma$", "and",
        "fractions", "$\\frac{a}{b}", "+", "\\frac{c}{d}$",
        "being", "added", "in", "real-time!",
        "This", "shows", "incremental", "parsing", "with", "math",
        "$\\alpha", "+", "\\beta", "=", "\\gamma$", "and",
        "fractions", "$\\frac{a}{b}", "+", "\\frac{c}{d}$",
        "being", "added", "in", "real-time!","This", "shows", "incremental", "parsing", "with", "math",
        "$\\alpha", "+", "\\beta", "=", "\\gamma$", "and",
        "fractions", "$\\frac{a}{b}", "+", "\\frac{c}{d}$",
        "being", "added", "in", "real-time!","This", "shows", "incremental", "parsing", "with", "math",
        "$\\alpha", "+", "\\beta", "=", "\\gamma$", "and",
        "fractions", "$\\frac{a}{b}", "+", "\\frac{c}{d}$",
        "being", "added", "in", "real-time!","This", "shows", "incremental", "parsing", "with", "math",
        "$\\alpha", "+", "\\beta", "=", "\\gamma$", "and",
        "fractions", "$\\frac{a}{b}", "+", "\\frac{c}{d}$",
        "being", "added", "in", "real-time!"
    });
    int* wordIndex = new int(0);

    // Create timer for streaming characters
    QTimer* streamTimer = new QTimer(&window);
    QObject::connect(streamTimer, &QTimer::timeout, [streamLabel, streamWords, wordIndex, streamTimer]() {
        if (*wordIndex < streamWords->length()) {
            QString nextWord = streamWords->at(*wordIndex) + " ";
            streamLabel->appendText(nextWord);
            (*wordIndex)++;
        } else {
            streamTimer->stop();
        }
    });

    // Start streaming after a 1 second delay
    QTimer::singleShot(1000, [streamTimer]() {
        streamTimer->start(16);
    });

    window.setCentralWidget(centralWidget);
    window.resize(800, 600);
    window.show();

    int retn = app.exec();

    // Clean up MicroTeX resources
    tex::LaTeX::release();
    return retn;
}
