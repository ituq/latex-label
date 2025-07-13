#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "LatexLabel.h"
#include <QScrollArea>
#include <string>
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

    window.setWindowTitle("Markdown Streaming Test");




    // Streaming markdown test
    LatexLabel* streamLabel = new LatexLabel(&window);
    streamLabel->setTextSize(20);
    streamLabel->setText(""); // Start empty
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidget(streamLabel);
    streamLabel->setMinimumSize(300, 1200);
    scroll->setWidgetResizable(true); // Make the scroll area resize its widget
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Ensure the scroll area expands


    streamLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Ensure the label expands
    window.setCentralWidget(scroll);


    // Enhanced markdown content with math expressions as a single string
    QString* testContent = new QString(
        "# h1 Heading 8-)\n"
        "## h2 Heading\n"
        "### h3 Heading\n"
        "#### h4 Heading\n"
        "##### h5 Heading\n"
        "###### h6 Heading\n\n"

        "# Sample Markdown\n\n"

        "This is some basic, sample markdown with math like $E = mc^2$.\n\n"

        "## Second Heading\n\n"

        "1. One with $\\alpha + \\beta = \\gamma$\n"
        "1. Two featuring $\\frac{1}{2} + \\frac{1}{3} = \\frac{5}{6}$\n"
        "1. Three showing $\\int_{0}^{1} x^2 dx = \\frac{1}{3}$\n\n"

        "> This is a longer blockquote with multiple sentences and mathematical expressions to test the indentation behavior. "
        "Here we have some mathematics: $\\lim_{x \\to 0} \\frac{\\sin x}{x} = 1$ and also $\\int_{0}^{\\infty} e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}$. "
        "The text should wrap properly while maintaining proper indentation throughout the entire blockquote section. "
        "Even with **bold** and *italic* formatting, the indentation should be preserved.\n\n"

        "And **bold**, *italics*, and even *italics and later **bold***. Even ~~strikethrough~~. "
        "[A link](https://markdowntohtml.com) to somewhere.\n\n"

        "Mathematical expressions like $\\sqrt{2} \\approx 1.414$ and $e^{i\\pi} + 1 = 0$ are beautiful.\n\n"

        "And code highlighting:\n\n"
        "```js\n"
        "var foo = 'bar';\n\n"
        "function baz(s) {\n"
        "  return foo + ':' + s;\n"
        "}\n"
        "```\n\n"

        "Or inline code like `var foo = 'bar';` with math $f(x) = x^2$.\n\n"

        "Some advanced math: $\\nabla \\cdot \\vec{E} = \\frac{\\rho}{\\epsilon_0}$ and $\\vec{F} = m\\vec{a}$.\n\n"

        "The end ... featuring $\\zeta(2) = \\frac{\\pi^2}{6}$"
    );
    QString* testContents = new QString(

        "> This is a longer blockquote with multiple sentences and mathematical expressions to test the indentation behavior. "
        "Here we have some mathematics: $\\lim_{x \\to 0} \\frac{\\sin x}{x} = 1$ and also $\\int_{0}^{\\infty} e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}$. "
        "The text should wrap properly while maintaining proper indentation throughout the entire blockquote section. "
        "Even with **bold** and *italic* formatting, the indentation should be preserved.\n\n"

    );

    QString test2 = R"CONTINUITY(Okay, let's break down continuity in functions. It's a fundamental concept in calculus, and while the definition can seem a bit technical, the idea is surprisingly intuitive.

    **1. The Intuitive Idea: No Jumps, Breaks, or Holes**

    Imagine a graph of a function.  A continuous function's graph looks like one you can draw
*without lifting your pen*.  There are no sudden jumps, breaks, or holes in the line.

    )CONTINUITY";
    //QString* testContent = &test2;
    // Split the content into words for streaming

    QStringList words = testContent->split(' ', Qt::SkipEmptyParts);
    int* wordIndex = new int(0);

    // Create timer for streaming words
    QTimer* streamTimer = new QTimer(&window);
    QObject::connect(streamTimer, &QTimer::timeout, [streamLabel, words, wordIndex, streamTimer]() {
        if (*wordIndex < words.length()) {
            QString nextWord = words.at(*wordIndex) + " ";
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

    window.resize(800, 600);
    window.show();




    int retn = app.exec();

    // Clean up MicroTeX resources
    tex::LaTeX::release();
    return retn;
}
