#include "CodeBlockWidget.h"
#include <algorithm>

QSize CodeBlockWidget::minimumSizeHint() const
{
    return sizeHint();
}

CodeBlockWidget::CodeBlockWidget(QWidget* parent)
    : QWidget(parent)
    , m_textLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_copyButton(nullptr)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_headerWidget(nullptr)
    , m_textAreaWidget(nullptr)
    , m_textAreaLayout(nullptr)
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
    , m_headerWidget(nullptr)
    , m_textAreaWidget(nullptr)
    , m_textAreaLayout(nullptr)
    , m_text(text)
    , m_language(language)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_font.setPointSize(font_size);
    setupUI();
}


void CodeBlockWidget::setupUI()
{

    int lineCount = std::max(1, static_cast<int>(m_text.count('\n')) + 1);
    // Create the container widget that will have the border
    //QWidget* container = new QWidget();
    //container->setObjectName("borderContainer");





    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    // Create header
    QWidget* header = new QWidget();
    header->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
    m_headerWidget = header;
    header->setContentsMargins(5,5,5,5);
    header->setObjectName("header");



    m_headerLayout = new QHBoxLayout(header);
    m_headerLayout->setContentsMargins(5, 2, 5, 2);
    QLabel* language_label = new QLabel(m_language,this);
    if(m_language=="")
        language_label->setText(QString("%1 Lines").arg(lineCount));


    m_headerLayout->addWidget(language_label);

    m_headerLayout->addStretch();

    m_copyButton = new QToolButton(this);
    m_copyButton->setText("Copy");
    m_copyButton->setCursor(Qt::PointingHandCursor);
    connect(m_copyButton, &QToolButton::clicked, this, &CodeBlockWidget::copyToClipboard);

    m_headerLayout->addWidget(m_copyButton);


    m_textLabel = new QLabel();
    m_textLabel->setTextFormat(Qt::PlainText);
    m_textLabel->setText(m_text);
    m_textLabel->setFont(m_font);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);


    QFontMetrics fm(m_font);

    int contentH = lineCount * fm.lineSpacing();
    m_textLabel->setFixedHeight(contentH);

    // Create scroll area
    QWidget* text_area = new QWidget();
    m_textAreaWidget = text_area;
    text_area->setContentsMargins(10,10,2,10);
    m_scrollArea = new QScrollArea(text_area);
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidget(m_textLabel);
    m_scrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);


    // Create layout for text_area with padding
    QVBoxLayout* textAreaLayout = new QVBoxLayout(text_area);
    m_textAreaLayout = textAreaLayout;
    textAreaLayout->addWidget(m_scrollArea);
    textAreaLayout->setContentsMargins(0,0,0,0);
    text_area->setObjectName("Frame");
    applyPaletteStyles();



    // Add widgets to the container's main layout
    m_mainLayout->addWidget(header);
    m_mainLayout->addWidget(text_area);


    // Clamp overall height to header + content height to avoid oversizing
    int spacing = m_mainLayout->spacing();
    int textMargins = m_textAreaWidget->contentsMargins().top() + m_textAreaWidget->contentsMargins().bottom()
                    + m_textAreaLayout->contentsMargins().top() + m_textAreaLayout->contentsMargins().bottom();
    int totalH = header->sizeHint().height() + spacing + contentH + textMargins;
    setFixedHeight(totalH);
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

    int headerH = m_headerWidget ? m_headerWidget->sizeHint().height() : 0;
    int lineCount = std::max(1, static_cast<int>(lines.count()));
    int contentH = lineCount * fm.lineSpacing();
    int textMargins = 0;
    if (m_textAreaWidget) {
        textMargins += m_textAreaWidget->contentsMargins().top() + m_textAreaWidget->contentsMargins().bottom();
    }
    if (m_textAreaLayout) {
        textMargins += m_textAreaLayout->contentsMargins().top() + m_textAreaLayout->contentsMargins().bottom();
    }
    int spacing = m_mainLayout ? m_mainLayout->spacing() : 0;
    int height = headerH + spacing + contentH + textMargins;
    qDebug()<<headerH+spacing+textMargins;
    int width = maxWidth + 2 * MARGIN + COPY_BUTTON_WIDTH + 20; // Extra space for scrollbars

    return QSize(width, height);
}



void CodeBlockWidget::updateLabelSize()
{
    QFontMetrics fm(m_font);
    int lineCount = std::max(1, static_cast<int>(m_text.count('\n')) + 1);
    int contentH = lineCount * fm.lineSpacing();
    if (m_textLabel) {
        m_textLabel->setFixedHeight(contentH);
    }
}

void CodeBlockWidget::copyToClipboard()
{
    QGuiApplication::clipboard()->setText(m_text);
}
