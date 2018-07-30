#pragma once
#if defined(KTEXTEDITOR)
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KTextEditor/ConfigInterface>
#else
#include <QPlainTextEdit>
#endif
#include <QVBoxLayout>
/**
 * This file contains two editors, one which uses KTextEditor
 * which features syntax highlighting, etc. and another, much simpler,
 * which uses a basic QPlainTextEdit.
 */
namespace isf_editor
{
#if defined(KTEXTEDITOR)
struct code_editor final
    : public QWidget
{
  Q_OBJECT
public:
  code_editor(QWidget* parent)
    : QWidget{parent}
    , m_lay{this}
  {
    // Note: see  https://api.kde.org/frameworks/ktexteditor/html/classKTextEditor_1_1ConfigInterface.htm
    // create a new document
    // create a widget to display the document

    m_lay.addWidget(m_view);
    m_doc->setHighlightingMode("GLSL");
    connect(m_doc, &KTextEditor::Document::textChanged, this,
            [=] (auto) { shaderChanged(m_doc->text()); });

    KTextEditor::ConfigInterface *iface =
        qobject_cast<KTextEditor::ConfigInterface*>( m_view );
    if( iface ) {
      iface->setConfigValue("Font", "Courier");
        // the implementation supports the interface
        // do stuff
    }
  }

  void setShader(QString s) { m_doc->setText(s); }
  QString shader() const { return m_doc->text(); }

Q_SIGNALS:
  void shaderChanged(QString);

private:
  QVBoxLayout m_lay;

  KTextEditor::Editor *m_edit = KTextEditor::Editor::instance();
  KTextEditor::Document *m_doc = m_edit->createDocument(this);
  KTextEditor::View *m_view = m_doc->createView(this);
};
#else
struct code_editor final
    : public QWidget
{
  Q_OBJECT
public:
  code_editor(QWidget* parent)
    : QWidget{parent}
    , m_lay{this}
  {
    m_lay.addWidget(&m_edit);
    connect(&m_edit, &QPlainTextEdit::textChanged,
            this,[=] () { shaderChanged(shader()); } );
  }

  void setShader(QString s) { m_edit.setPlainText(s); }
  QString shader() const { return m_edit.document()->toPlainText(); }

Q_SIGNALS:
  void shaderChanged(QString);

private:
  QVBoxLayout m_lay;

  QPlainTextEdit m_edit;
};
#endif
}
