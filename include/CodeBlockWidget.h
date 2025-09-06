#pragma once

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QFontMetrics>
#include <QString>
#include <QGuiApplication>
#include <QClipboard>
#include <QFrame>
#include <QPalette>
#include <QEvent>

class CodeBlockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CodeBlockWidget(QWidget* parent = nullptr);
    explicit CodeBlockWidget(const QString& text,int font_size, const QString& language = QString(), QWidget* parent = nullptr);



    // Size management
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;


protected:
    void changeEvent(QEvent* event) override;
private slots:
    void copyToClipboard();

private:
    void setupUI();
    void updateLabelSize();
    void applyPaletteStyles();

    QLabel* m_textLabel;
    QLabel* m_languageLabel;
    QScrollArea* m_scrollArea;
    QToolButton* m_copyButton;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QWidget* m_headerWidget;
    QWidget* m_textAreaWidget;
    QVBoxLayout* m_textAreaLayout;

    QString m_text;
    QString m_language;
    QFont m_font= QFont("Monaco");

    static constexpr int MARGIN = 8;
    static constexpr int COPY_BUTTON_WIDTH = 44;
    static constexpr int COPY_BUTTON_HEIGHT = 24;
};

inline void CodeBlockWidget::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ApplicationPaletteChange ||
        event->type() == QEvent::StyleChange) {
        applyPaletteStyles();
    }
}

inline void CodeBlockWidget::applyPaletteStyles()
{

    const QPalette pal = QGuiApplication::palette();

    if (m_headerWidget) {
        m_headerWidget->setStyleSheet(QString(R"(
        QWidget#header {
            background-color: %1;
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
        }
    )").arg(pal.base().color().darker(120).name()));
    }
    if (m_textAreaWidget) {
        m_textAreaWidget->setStyleSheet(QString(R"(
        QWidget#Frame {
            border-bottom-left-radius: 10px;
            border-bottom-right-radius: 10px;
            border: 2px solid %1;
            background-color: palette(base);
        }
    )").arg(pal.base().color().darker(120).name()));
    }
    if( m_textLabel)
        m_textLabel->setStyleSheet("QWidget{color: palette(text); background-color: palette(base);}");
    if(m_scrollArea)
        m_scrollArea->setStyleSheet("QWidget#scrollArea{background-color: palette(base);}");
    if(m_copyButton){
        m_copyButton->setStyleSheet("QWidget{background-color: palette(button);color: palette(text);}");
    }
    if(m_languageLabel){
        m_languageLabel->setStyleSheet(QString("QWidget{background-color: %1; color: palette(text);}")
            .arg(pal.base().color().darker(120).name()));
    }
}
