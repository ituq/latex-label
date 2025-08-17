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

class CodeBlockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CodeBlockWidget(QWidget* parent = nullptr);
    explicit CodeBlockWidget(const QString& text,int font_size, const QString& language = QString(), QWidget* parent = nullptr);



    // Size management
    QSize sizeHint() const override;


private slots:
    void copyToClipboard();

private:
    void setupUI();
    void updateLabelSize();

    QLabel* m_textLabel;
    QScrollArea* m_scrollArea;
    QToolButton* m_copyButton;
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;

    QString m_text;
    QString m_language;
    QFont m_font= QFont("Monaco");

    static constexpr int MARGIN = 8;
    static constexpr int COPY_BUTTON_WIDTH = 44;
    static constexpr int COPY_BUTTON_HEIGHT = 24;
};
