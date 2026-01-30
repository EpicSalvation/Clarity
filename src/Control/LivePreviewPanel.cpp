#include "LivePreviewPanel.h"
#include <QVBoxLayout>
#include <QLabel>

namespace Clarity {

LivePreviewPanel::LivePreviewPanel(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(10);

    // Panel title
    QLabel* titleLabel = new QLabel(tr("Live Preview"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    // Output preview
    m_outputPreview = new LivePreviewWidget(tr("Output"), this);
    layout->addWidget(m_outputPreview);

    // Confidence preview (shows current + next slide)
    m_confidencePreview = new ConfidencePreviewWidget(this);
    layout->addWidget(m_confidencePreview);

    // Add stretch to push previews to the top
    layout->addStretch();

    // Set a fixed width for the panel
    setFixedWidth(200);
}

void LivePreviewPanel::setSettingsManager(SettingsManager* settings)
{
    m_confidencePreview->setSettingsManager(settings);
}

void LivePreviewPanel::setSlides(const Slide& currentSlide, const Slide& nextSlide,
                                  int currentIndex, int totalSlides)
{
    m_outputPreview->setSlide(currentSlide);
    m_confidencePreview->setSlides(currentSlide, nextSlide, currentIndex, totalSlides);
}

void LivePreviewPanel::setOutputSlide(const Slide& slide)
{
    m_outputPreview->setSlide(slide);
}

void LivePreviewPanel::setConfidenceSlides(const Slide& currentSlide, const Slide& nextSlide,
                                            int currentIndex, int totalSlides)
{
    m_confidencePreview->setSlides(currentSlide, nextSlide, currentIndex, totalSlides);
}

void LivePreviewPanel::clearOutput()
{
    m_outputPreview->clear();
}

void LivePreviewPanel::clearConfidence()
{
    m_confidencePreview->clear();
}

void LivePreviewPanel::clearAll()
{
    m_outputPreview->clear();
    m_confidencePreview->clear();
}

QSize LivePreviewPanel::sizeHint() const
{
    return QSize(200, 450);
}

QSize LivePreviewPanel::minimumSizeHint() const
{
    return QSize(180, 380);
}

} // namespace Clarity
