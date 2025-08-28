#include "CodeBlockWidget.h"
#include <algorithm>

CodeBlockWidget::CodeBlockWidget(QWidget* parent)
    : QWidget(parent)
    , m_textLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_copyButton(nullptr)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
{
    setupUI();
}

CodeBlockWidget::CodeBlockWidget(const QString& text,int font_size, const QString& language, QWidget* parent)
    : QWidget(parent)
    , m_textLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_copyButton(nullptr)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_text(text)
    , m_language(language)
{
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Ignored);
    m_font.setPointSize(font_size);
    setupUI();
}


void CodeBlockWidget::setupUI()
{
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);

    QPalette pal= QGuiApplication::palette();
    // Create header
    QWidget* header = new QWidget();
    header->setStyleSheet(QString(R"(
        background-color: %1;
    )").arg(pal.alternateBase().color().name()));
    m_headerLayout = new QHBoxLayout(header);
    m_headerLayout->setContentsMargins(MARGIN, MARGIN, MARGIN, 0);

    QLabel* language_label = new QLabel(m_language,this);
    m_headerLayout->addWidget(language_label);

    // Add spacer to push copy button to the right
    m_headerLayout->addStretch();

    // Create copy button
    m_copyButton = new QToolButton(this);
    m_copyButton->setText("Copy");
    m_copyButton->setCursor(Qt::PointingHandCursor);
    connect(m_copyButton, &QToolButton::clicked, this, &CodeBlockWidget::copyToClipboard);

    m_headerLayout->addWidget(m_copyButton);

    // Create text label
    m_textLabel = new QLabel(this);
    m_textLabel->setTextFormat(Qt::PlainText);
    m_textLabel->setText(m_text);
    m_textLabel->setFont(m_font);
    m_textLabel->setMargin(10);
    m_textLabel->setObjectName("scrollAreaContent");

    QFontMetrics fm(m_font);
    int lines= m_text.split('\n').length();

    /*

    m_textLabel->setMinimumHeight(lines*fm.lineSpacing());
    */

    // Create scroll area
    QWidget* text_area = new QWidget(this);
    m_scrollArea = new QScrollArea(text_area);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidget(m_textLabel);
    m_scrollArea->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    
    // Create layout for text_area with padding
    QVBoxLayout* textAreaLayout = new QVBoxLayout(text_area);
    textAreaLayout->setContentsMargins(10, 10, 10, 10); // Add padding
    textAreaLayout->addWidget(m_scrollArea);
    
    text_area->setObjectName("border");
    text_area->setStyleSheet(QString(R"(
        QWidget#border {
            border-radius: 10px;
            border: 2px solid %1;
        }
    )").arg(pal.base().color().darker(75).name()));




    // Add layouts to main layout
    m_mainLayout->addWidget(header);
    m_mainLayout->addWidget(text_area);

    // Apply text, font, and styling

}




QSize CodeBlockWidget::sizeHint() const
{
    if (!m_textLabel || m_text.isEmpty()) {
        return QSize(200, 100);
    }

    QFontMetrics fm(m_font);
    QStringList lines = m_text.split('\n');

    int maxWidth = 0;
    for (const QString& line : lines) {
        maxWidth = std::max(maxWidth, fm.horizontalAdvance(line));
    }

    int height = fm.lineSpacing() * std::max(1, static_cast<int>(lines.count())) + 2 * MARGIN + COPY_BUTTON_HEIGHT;
    int width = maxWidth + 2 * MARGIN + COPY_BUTTON_WIDTH + 20; // Extra space for scrollbars

    return QSize(width, height);
}



void CodeBlockWidget::copyToClipboard()
{
    QGuiApplication::clipboard()->setText(m_text);
}
