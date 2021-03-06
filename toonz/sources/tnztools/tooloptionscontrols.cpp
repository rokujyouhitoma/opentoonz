

// TnzCore includes
#include "tpalette.h"
#include "tcolorstyles.h"
#include "tundo.h"

// TnzBase includes
#include "tproperty.h"

// TnzLib includes
#include "toonz/palettecontroller.h"
#include "toonz/tpalettehandle.h"
#include "toonz/tobjecthandle.h"
#include "toonz/stage2.h"
#include "toonz/stageobjectutil.h"
#include "toonz/doubleparamcmd.h"

// TnzQt includes
#include "toonzqt/gutil.h"

// TnzTools includes
#include "tools/tool.h"
#include "rasterselectiontool.h"
#include "vectorselectiontool.h"
// to enable the increase/decrease shortcuts while hiding the tool option
#include "tools/toolhandle.h"
// to enable shortcuts only when the viewer is focused
#include "tools/tooloptions.h"

// Qt includes
#include <QPainter>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QButtonGroup>
#include <QMenu>

#include "tooloptionscontrols.h"

using namespace DVGui;

//***********************************************************************************
//    ToolOptionControl  implementation
//***********************************************************************************

ToolOptionControl::ToolOptionControl(TTool *tool, string propertyName, ToolHandle *toolHandle)
	: m_tool(tool), m_propertyName(propertyName), m_toolHandle(toolHandle)
{
}

//-----------------------------------------------------------------------------

void ToolOptionControl::notifyTool()
{
	m_tool->onPropertyChanged(m_propertyName);
}

//-----------------------------------------------------------------------------
/*! return true if the control is belonging to the visible combo viewer. very dirty implementation.
*/
bool ToolOptionControl::isInVisibleViewer(QWidget *widget)
{
	if (!widget)
		return false;

	if (widget->isVisible())
		return true;

	ToolOptionsBox *parentTOB = dynamic_cast<ToolOptionsBox *>(widget->parentWidget());
	if (!parentTOB)
		return false;

	ToolOptions *grandParentTO = dynamic_cast<ToolOptions *>(parentTOB->parentWidget());
	if (!grandParentTO)
		return false;

	//There must be a QFrame between a ComboViewerPanel and a ToolOptions
	QFrame *greatGrandParentFrame = dynamic_cast<QFrame *>(grandParentTO->parentWidget());
	if (!greatGrandParentFrame)
		return false;

	return greatGrandParentFrame->isVisible();
}

//***********************************************************************************
//    ToolOptionControl derivative  implementations
//***********************************************************************************

ToolOptionCheckbox::ToolOptionCheckbox(TTool *tool, TBoolProperty *property, ToolHandle *toolHandle,
									   QWidget *parent)
	: CheckBox(parent), ToolOptionControl(tool, property->getName(), toolHandle), m_property(property)
{
	setText(property->getQStringName());
	m_property->addListener(this);
	updateStatus();
	// synchronize the state with the same widgets in other tool option bars
	if (toolHandle)
		connect(this, SIGNAL(clicked(bool)), toolHandle, SIGNAL(toolChanged()));
}

//-----------------------------------------------------------------------------

void ToolOptionCheckbox::updateStatus()
{
	bool check = m_property->getValue();
	if (isChecked() == check)
		return;

	setCheckState(check ? Qt::Checked : Qt::Unchecked);
}

//-----------------------------------------------------------------------------

void ToolOptionCheckbox::nextCheckState()
{
	QAbstractButton::nextCheckState();
	m_property->setValue(checkState() == Qt::Checked);
	notifyTool();
}

//-----------------------------------------------------------------------------

void ToolOptionCheckbox::doClick()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;
	click();
}

//=============================================================================

ToolOptionSlider::ToolOptionSlider(TTool *tool, TDoubleProperty *property, ToolHandle *toolHandle)
	: DoubleField(), ToolOptionControl(tool, property->getName(), toolHandle), m_property(property)
{
	m_property->addListener(this);
	TDoubleProperty::Range range = property->getRange();
	setRange(range.first, range.second);
	updateStatus();
	connect(this, SIGNAL(valueChanged(bool)), SLOT(onValueChanged(bool)));
	// synchronize the state with the same widgets in other tool option bars
	if (toolHandle)
		connect(this, SIGNAL(valueEditedByHand()), toolHandle, SIGNAL(toolChanged()));
}

//-----------------------------------------------------------------------------

void ToolOptionSlider::updateStatus()
{
	double v = m_property->getValue();
	if (getValue() == v)
		return;

	setValue(v);
}

//-----------------------------------------------------------------------------

void ToolOptionSlider::onValueChanged(bool isDragging)
{
	m_property->setValue(getValue());
	notifyTool();
}

//-----------------------------------------------------------------------------

void ToolOptionSlider::increase()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	double value = getValue();
	double minValue, maxValue;
	getRange(minValue, maxValue);

	value += 1;
	if (value > maxValue)
		value = maxValue;

	setValue(value);
	m_property->setValue(getValue());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionSlider::decrease()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	double value = getValue();
	double minValue, maxValue;
	getRange(minValue, maxValue);

	value -= 1;
	if (value < minValue)
		value = minValue;

	setValue(value);
	m_property->setValue(getValue());
	notifyTool();
	//update the interface
	repaint();
}

//=============================================================================

ToolOptionPairSlider::ToolOptionPairSlider(TTool *tool, TDoublePairProperty *property,
										   const QString &leftName, const QString &rightName,
										   ToolHandle *toolHandle)
	: DoublePairField(0, property->isMaxRangeLimited()), ToolOptionControl(tool, property->getName(), toolHandle), m_property(property)
{
	setLeftText(leftName);
	setRightText(rightName);
	m_property->addListener(this);
	TDoublePairProperty::Value value = property->getValue();
	TDoublePairProperty::Range range = property->getRange();
	setRange(range.first, range.second);
	updateStatus();
	connect(this, SIGNAL(valuesChanged(bool)), SLOT(onValuesChanged(bool)));
}

//-----------------------------------------------------------------------------

void ToolOptionPairSlider::updateStatus()
{
	TDoublePairProperty::Value value = m_property->getValue();
	setValues(value);
}

//-----------------------------------------------------------------------------

void ToolOptionPairSlider::onValuesChanged(bool isDragging)
{
	m_property->setValue(getValues());
	notifyTool();
	// synchronize the state with the same widgets in other tool option bars
	if (m_toolHandle)
		m_toolHandle->notifyToolChanged();
}

//-----------------------------------------------------------------------------

void ToolOptionPairSlider::increaseMaxValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<double, double> values = getValues();
	double minValue, maxValue;
	getRange(minValue, maxValue);
	values.second += 1;
	if (values.second > maxValue)
		values.second = maxValue;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionPairSlider::decreaseMaxValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<double, double> values = getValues();
	values.second -= 1;
	if (values.second < values.first)
		values.second = values.first;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionPairSlider::increaseMinValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<double, double> values = getValues();
	values.first += 1;
	if (values.first > values.second)
		values.first = values.second;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionPairSlider::decreaseMinValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<double, double> values = getValues();
	double minValue, maxValue;
	getRange(minValue, maxValue);
	values.first -= 1;
	if (values.first < minValue)
		values.first = minValue;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//=============================================================================

ToolOptionIntPairSlider::ToolOptionIntPairSlider(TTool *tool, TIntPairProperty *property,
												 const QString &leftName, const QString &rightName,
												 ToolHandle *toolHandle)
	: IntPairField(0, property->isMaxRangeLimited()), ToolOptionControl(tool, property->getName(), toolHandle), m_property(property)
{
	setLeftText(leftName);
	setRightText(rightName);
	m_property->addListener(this);
	TIntPairProperty::Value value = property->getValue();
	TIntPairProperty::Range range = property->getRange();
	setRange(range.first, range.second);
	updateStatus();
	connect(this, SIGNAL(valuesChanged(bool)), SLOT(onValuesChanged(bool)));
}

//-----------------------------------------------------------------------------

void ToolOptionIntPairSlider::updateStatus()
{
	TIntPairProperty::Value value = m_property->getValue();
	setValues(value);
}

//-----------------------------------------------------------------------------

void ToolOptionIntPairSlider::onValuesChanged(bool isDragging)
{
	m_property->setValue(getValues());
	notifyTool();
	// synchronize the state with the same widgets in other tool option bars
	if (m_toolHandle)
		m_toolHandle->notifyToolChanged();
}

//-----------------------------------------------------------------------------

void ToolOptionIntPairSlider::increaseMaxValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<int, int> values = getValues();
	int minValue, maxValue;
	getRange(minValue, maxValue);
	values.second += 1;

	// a "cross-like shape" of the brush size = 3 is hard to use. so skip it
	if (values.second == 3 && m_tool->isPencilModeActive())
		values.second += 1;

	if (values.second > maxValue)
		values.second = maxValue;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionIntPairSlider::decreaseMaxValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<int, int> values = getValues();
	values.second -= 1;

	// a "cross-like shape" of the brush size = 3 is hard to use. so skip it
	if (values.second == 3 && m_tool->isPencilModeActive())
		values.second -= 1;

	if (values.second < values.first)
		values.second = values.first;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionIntPairSlider::increaseMinValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<int, int> values = getValues();
	values.first += 1;
	if (values.first > values.second)
		values.first = values.second;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionIntPairSlider::decreaseMinValue()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	std::pair<int, int> values = getValues();
	int minValue, maxValue;
	getRange(minValue, maxValue);
	values.first -= 1;
	if (values.first < minValue)
		values.first = minValue;
	setValues(values);
	m_property->setValue(getValues());
	notifyTool();
	//update the interface
	repaint();
}

//=============================================================================

ToolOptionIntSlider::ToolOptionIntSlider(TTool *tool, TIntProperty *property,
										 ToolHandle *toolHandle)
	: IntField(0, property->isMaxRangeLimited()), ToolOptionControl(tool, property->getName(), toolHandle), m_property(property)
{
	m_property->addListener(this);
	TIntProperty::Range range = property->getRange();
	setRange(range.first, range.second);
	updateStatus();
	connect(this, SIGNAL(valueChanged(bool)), SLOT(onValueChanged(bool)));
	// synchronize the state with the same widgets in other tool option bars
	if (toolHandle)
		connect(this, SIGNAL(valueEditedByHand()), toolHandle, SIGNAL(toolChanged()));
}

//-----------------------------------------------------------------------------

void ToolOptionIntSlider::updateStatus()
{
	int v = m_property->getValue();
	if (getValue() == v)
		return;

	setValue(v);
}

//-----------------------------------------------------------------------------

void ToolOptionIntSlider::increase()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	int value = getValue();
	int minValue, maxValue;
	getRange(minValue, maxValue);
	value += 1;
	// a "cross-like shape" of the brush size = 3 is hard to use. so skip it
	if (value == 3 && m_tool->isPencilModeActive())
		value += 1;

	if (value > maxValue)
		value = maxValue;

	setValue(value);
	m_property->setValue(getValue());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionIntSlider::decrease()
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	int value = getValue();
	int minValue, maxValue;
	getRange(minValue, maxValue);
	value -= 1;

	// a "cross-like shape" of the brush size = 3 is hard to use. so skip it
	if (value == 3 && m_tool->isPencilModeActive())
		value -= 1;

	if (value < minValue)
		value = minValue;

	setValue(value);
	m_property->setValue(getValue());
	notifyTool();
	//update the interface
	repaint();
}

//-----------------------------------------------------------------------------

void ToolOptionIntSlider::onValueChanged(bool isDragging)
{
	m_property->setValue(getValue());
	notifyTool();
}

//=============================================================================

ToolOptionCombo::ToolOptionCombo(TTool *tool, TEnumProperty *property, ToolHandle *toolHandle)
	: QComboBox(), ToolOptionControl(tool, property->getName(), toolHandle), m_property(property)
{
	setMinimumWidth(65);
	m_property->addListener(this);
	loadEntries();
	setSizeAdjustPolicy(QComboBox::AdjustToContents);
	connect(this, SIGNAL(activated(int)), this, SLOT(onActivated(int)));
	// synchronize the state with the same widgets in other tool option bars
	if (toolHandle)
		connect(this, SIGNAL(activated(int)), toolHandle, SIGNAL(toolChanged()));
}

//-----------------------------------------------------------------------------

void ToolOptionCombo::loadEntries()
{
	TEnumProperty::Range range = m_property->getRange();
	TEnumProperty::Range::iterator it;

	clear();
	for (it = range.begin(); it != range.end(); ++it)
		addItem(QString::fromStdWString(*it));

	updateStatus();
}

//-----------------------------------------------------------------------------

void ToolOptionCombo::updateStatus()
{
	QString value = QString::fromStdWString(m_property->getValue());
	int index = findText(value);
	if (index >= 0 && index != currentIndex())
		setCurrentIndex(index);
}

//-----------------------------------------------------------------------------

void ToolOptionCombo::onActivated(int index)
{
	const TEnumProperty::Range &range = m_property->getRange();
	if (index < 0 || index >= (int)range.size())
		return;

	wstring item = range[index];
	m_property->setValue(item);
	notifyTool();
}

//-----------------------------------------------------------------------------

void ToolOptionCombo::doShowPopup()
{
	if (isVisible())
		showPopup();
}

//-----------------------------------------------------------------------------

void ToolOptionCombo::doOnActivated(int index)
{
	if (m_toolHandle && m_toolHandle->getTool() != m_tool)
		return;
	// active only if the belonging combo-viewer is visible
	if (!isInVisibleViewer(this))
		return;

	// Just move the index if the first item is not "Normal"
	if (itemText(0) != "Normal") {
		onActivated(index);
		setCurrentIndex(index);
		//for updating the cursor
		m_toolHandle->notifyToolChanged();
		return;
	}

	// If the first item of this combo box is "Normal", enable shortcut key toggle can "back and forth" behavior.
	if (currentIndex() == index) {
		// estimating that the "Normal" option is located at the index 0
		onActivated(0);
		setCurrentIndex(0);
	} else {
		onActivated(index);
		setCurrentIndex(index);
	}

	// for updating a cursor without any effect to the tool options
	m_toolHandle->notifyToolCursorTypeChanged();
}

//=============================================================================

ToolOptionPopupButton::ToolOptionPopupButton(TTool *tool, TEnumProperty *property)
	: PopupButton(), ToolOptionControl(tool, property->getName()), m_property(property)
{
	setObjectName(QString::fromStdString(property->getName()));
	setFixedHeight(20);
	m_property->addListener(this);

	TEnumProperty::Range range = property->getRange();
	TEnumProperty::Range::iterator it;
	for (it = range.begin(); it != range.end(); ++it)
		addItem(createQIconPNG(QString::fromStdWString(*it).toUtf8()));

	setCurrentIndex(0);
	updateStatus();
	connect(this, SIGNAL(activated(int)), this, SLOT(onActivated(int)));
}

//-----------------------------------------------------------------------------

void ToolOptionPopupButton::updateStatus()
{
	int index = m_property->getIndex();
	if (index >= 0 && index != currentIndex())
		setCurrentIndex(index);
}

//-----------------------------------------------------------------------------

void ToolOptionPopupButton::onActivated(int index)
{
	const TEnumProperty::Range &range = m_property->getRange();
	if (index < 0 || index >= (int)range.size())
		return;

	wstring item = range[index];
	m_property->setValue(item);
	notifyTool();
}

//-----------------------------------------------------------------------------

void ToolOptionPopupButton::doShowPopup()
{
	if (isVisible())
		showMenu();
}

//-----------------------------------------------------------------------------

void ToolOptionPopupButton::doSetCurrentIndex(int index)
{
	if (isVisible())
		setCurrentIndex(index);
}

//-----------------------------------------------------------------------------

void ToolOptionPopupButton::doOnActivated(int index)
{
	if (isVisible())
		onActivated(index);
}

//=============================================================================

ToolOptionTextField::ToolOptionTextField(TTool *tool, TStringProperty *property)
	: LineEdit(), ToolOptionControl(tool, property->getName()), m_property(property)
{
	setFixedSize(100, 23);
	m_property->addListener(this);

	updateStatus();
	connect(this, SIGNAL(editingFinished()), SLOT(onValueChanged()));
}

//-----------------------------------------------------------------------------

void ToolOptionTextField::updateStatus()
{
	QString newText = QString::fromStdWString(m_property->getValue());
	if (newText == text())
		return;

	setText(newText);
}

//-----------------------------------------------------------------------------

void ToolOptionTextField::onValueChanged()
{
	m_property->setValue(text().toStdWString());
	notifyTool();
	// synchronize the state with the same widgets in other tool option bars
	if (m_toolHandle)
		m_toolHandle->notifyToolChanged();
}

//=============================================================================

StyleIndexFieldAndChip::StyleIndexFieldAndChip(TTool *tool, TStyleIndexProperty *property, TPaletteHandle *pltHandle,
											   ToolHandle *toolHandle)
	: StyleIndexLineEdit(), ToolOptionControl(tool, property->getName(), toolHandle), m_property(property), m_pltHandle(pltHandle)
{
	m_property->addListener(this);

	updateStatus();
	connect(this, SIGNAL(textChanged(const QString &)), SLOT(onValueChanged(const QString &)));

	setPaletteHandle(pltHandle);
	connect(pltHandle, SIGNAL(colorStyleSwitched()), SLOT(updateColor()));
	connect(pltHandle, SIGNAL(colorStyleChanged()), SLOT(updateColor()));
}

//-----------------------------------------------------------------------------

void StyleIndexFieldAndChip::updateStatus()
{
	QString newText = QString::fromStdWString(m_property->getValue());
	if (newText == text())
		return;

	setText(newText);
}

//-----------------------------------------------------------------------------

void StyleIndexFieldAndChip::onValueChanged(const QString &changedText)
{
	QString style;

	if (!QString("current").contains(changedText)) {
		int index = changedText.toInt();
		TPalette *plt = m_pltHandle->getPalette();
		int indexCount = plt->getStyleCount();
		if (index > indexCount)
			style = QString::number(indexCount - 1);
		else
			style = text();
	}
	m_property->setValue(style.toStdWString());
	repaint();
	// synchronize the state with the same widgets in other tool option bars
	if (m_toolHandle)
		m_toolHandle->notifyToolChanged();
}

//-----------------------------------------------------------------------------

void StyleIndexFieldAndChip::updateColor()
{
	repaint();
}

//=============================================================================

ToolOptionParamRelayField::ToolOptionParamRelayField(
	TTool *tool, TDoubleParamRelayProperty *property, int decimals)
	: MeasuredDoubleLineEdit(), ToolOptionControl(tool, property->getName()), m_param(), m_measure(), m_property(property), m_globalKey(), m_globalGroup()
{
	setFixedSize(70, 20);
	m_property->addListener(this);

	setDecimals(decimals);
	updateStatus();
	connect(this, SIGNAL(valueChanged()), SLOT(onValueChanged()));
}

//-----------------------------------------------------------------------------

void ToolOptionParamRelayField::setGlobalKey(TBoolProperty *globalKey, TPropertyGroup *globalGroup)
{
	m_globalKey = globalKey, m_globalGroup = globalGroup;
}

//-----------------------------------------------------------------------------

void ToolOptionParamRelayField::updateStatus()
{
	TDoubleParamP param(m_property->getParam());
	if (param != m_param) {
		// Initialize param referencing
		m_param = param.getPointer();

		if (param) {
			m_measure = param->getMeasure();
			setMeasure(m_measure ? m_measure->getName() : "");

			setValue(m_property->getValue());
		}
	}

	if (!param) {
		setEnabled(false);
		m_measure = 0;
		setText("");

		return;
	}

	setEnabled(true);

	TMeasure *measure = param->getMeasure();
	if (measure != m_measure) {
		// Update measure if needed
		m_measure = measure;
		setMeasure(measure ? measure->getName() : "");
	}

	double v = m_property->getValue();
	if (getValue() == v)
		return;

	// Update value if needed
	setValue(v);
}

//-----------------------------------------------------------------------------

void ToolOptionParamRelayField::onValueChanged()
{
	struct locals {
		static inline void setKeyframe(TDoubleParamRelayProperty *prop)
		{
			if (!prop)
				return;

			TDoubleParam *param = prop->getParam().getPointer();
			if (!param)
				return;

			double frame = prop->frame();
			if (!param->isKeyframe(frame)) {
				KeyframeSetter setter(param, -1, true);
				setter.createKeyframe(frame);
			}
		}

		//-----------------------------------------------------------------------------

		struct SetValueUndo : public TUndo {
			TDoubleParamP m_param;	 //!< The referenced param
			double m_oldVal, m_newVal; //!< Values before and after the set action...
			double m_frame;			   //!< ... at this frame

		public:
			SetValueUndo(const TDoubleParamP &param, double oldVal, double newVal, double frame)
				: m_param(param), m_oldVal(oldVal), m_newVal(newVal), m_frame(frame) {}

			int getSize() const { return sizeof(SetValueUndo) + sizeof(TDoubleParam); }
			void undo() const { m_param->setValue(m_frame, m_oldVal); }
			void redo() const { m_param->setValue(m_frame, m_newVal); }
		};
	};

	//-----------------------------------------------------------------------------

	// NOTE: Values are extracted upon entry, since setting a keyframe reverts the lineEdit
	// field value back to the original value (due to feedback from the param itself)...
	double oldVal = m_property->getValue(), newVal = getValue(), frame = m_property->frame();

	TDoubleParamP param = m_property->getParam();
	if (!param)
		return;

	TUndoManager *manager = TUndoManager::manager();
	manager->beginBlock();

	if (m_globalKey && m_globalGroup && m_globalKey->getValue()) {
		// Set a keyframe for each of the TDoubleParam relayed in the globalGroup
		int p, pCount = m_globalGroup->getPropertyCount();
		for (p = 0; p != pCount; ++p) {
			TProperty *prop = m_globalGroup->getProperty(p);
			if (TDoubleParamRelayProperty *relProp = dynamic_cast<TDoubleParamRelayProperty *>(prop))
				locals::setKeyframe(relProp);
		}
	} else {
		// Set a keyframe just for our param
		locals::setKeyframe(m_property);
	}

	// Assign the edited value to the relayed param
	m_property->setValue(newVal);
	notifyTool();

	manager->add(new locals::SetValueUndo(param, oldVal, newVal, frame));
	manager->endBlock();
}

//=============================================================================
//
// Widget specifici di ArrowTool (derivati da ToolOptionControl)
//

// SPOSTA in un file a parte!

using namespace DVGui;

MeasuredValueField::MeasuredValueField(QWidget *parent, QString name)
	: LineEdit(name, parent), m_isGlobalKeyframe(false), m_modified(false), m_errorHighlighting(false), m_precision(2)
{
	setObjectName("MeasuredValueField");

	m_value = new TMeasuredValue("length");
	setText(QString::fromStdWString(m_value->toWideString(m_precision)));
	connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(onTextChanged(const QString &)));
	connect(this, SIGNAL(editingFinished()), SLOT(commit()));
	connect(&m_errorHighlightingTimer, SIGNAL(timeout()), this, SLOT(errorHighlightingTick()));
}

//-----------------------------------------------------------------------------

MeasuredValueField::~MeasuredValueField()
{
	delete m_value;
}

//-----------------------------------------------------------------------------

void MeasuredValueField::setMeasure(string name)
{
	// for reproducing the precision
	int oldPrec = -1;

	delete m_value;
	m_value = new TMeasuredValue(name != "" ? name : "dummy");

	setText(QString::fromStdWString(m_value->toWideString(m_precision)));
}

//-----------------------------------------------------------------------------

void MeasuredValueField::commit()
{
	if (!m_modified && !isReturnPressed())
		return;
	int err = 1;
	bool isSet = m_value->setValue(text().toStdWString(), &err);
	m_modified = false;
	if (err != 0) {
		setText(QString::fromStdWString(m_value->toWideString(m_precision)));
		m_errorHighlighting = 1;
		if (!m_errorHighlightingTimer.isActive())
			m_errorHighlightingTimer.start(40);
	} else {
		if (m_errorHighlightingTimer.isActive())
			m_errorHighlightingTimer.stop();
		m_errorHighlighting = 0;
		setStyleSheet("");
	}

	if (!isSet && !isReturnPressed())
		return;

	setText(QString::fromStdWString(m_value->toWideString(m_precision)));
	m_modified = false;
	emit measuredValueChanged(m_value);
}

//-----------------------------------------------------------------------------

void MeasuredValueField::onTextChanged(const QString &)
{
	m_modified = true;
}

//-----------------------------------------------------------------------------

void MeasuredValueField::setValue(double v)
{
	if (getValue() == v)
		return;
	m_value->setValue(TMeasuredValue::MainUnit, v);
	setText(QString::fromStdWString(m_value->toWideString(m_precision)));
}

//-----------------------------------------------------------------------------

double MeasuredValueField::getValue() const
{
	return m_value->getValue(TMeasuredValue::MainUnit);
}

//-----------------------------------------------------------------------------

void MeasuredValueField::errorHighlightingTick()
{
	if (m_errorHighlighting < 0.01) {
		if (m_errorHighlightingTimer.isActive())
			m_errorHighlightingTimer.stop();
		m_errorHighlighting = 0;
		setStyleSheet("");
	} else {
		int v = 255 - (int)(m_errorHighlighting * 255);
		m_errorHighlighting = m_errorHighlighting * 0.8;
		int c = 255 << 16 | v << 8 | v;
		setStyleSheet(QString("#MeasuredValueField {background-color:#%1}").arg(c, 6, 16, QLatin1Char('0')));
	}
}

//-----------------------------------------------------------------------------

void MeasuredValueField::setPrecision(int precision)
{
	m_precision = precision;
	setText(QString::fromStdWString(m_value->toWideString(m_precision)));
}

//=============================================================================

PegbarChannelField::PegbarChannelField(TTool *tool, enum TStageObject::Channel actionId, QString name,
									   TFrameHandle *frameHandle, TObjectHandle *objHandle, TXsheetHandle *xshHandle,
									   QWidget *parent)
	: MeasuredValueField(parent, name), ToolOptionControl(tool, ""), m_actionId(actionId), m_frameHandle(frameHandle), m_objHandle(objHandle), m_xshHandle(xshHandle), m_scaleType(eNone)
{
	bool ret = connect(this, SIGNAL(measuredValueChanged(TMeasuredValue *)),
					   SLOT(onChange(TMeasuredValue *)));
	assert(ret);
	// NOTA: per le unita' di misura controlla anche tpegbar.cpp
	switch (actionId) {
	case TStageObject::T_X:
		setMeasure("length.x");
		break;
	case TStageObject::T_Y:
		setMeasure("length.y");
		break;
	case TStageObject::T_Z:
		setMeasure("zdepth");
		break;
	case TStageObject::T_Path:
		setMeasure("percentage2");
		break;
	case TStageObject::T_ShearX:
	case TStageObject::T_ShearY:
		setMeasure("shear");
		break;
	case TStageObject::T_Angle:
		setMeasure("angle");
		break;
	case TStageObject::T_ScaleX:
	case TStageObject::T_ScaleY:
	case TStageObject::T_Scale:
		setMeasure("scale");
		break;
	default:
		setMeasure("dummy");
		break;
	}
	updateStatus();
}

//-----------------------------------------------------------------------------

void PegbarChannelField::onChange(TMeasuredValue *fld)
{
	if (!m_tool->isEnabled())
		return;
	TUndoManager::manager()->beginBlock();
	TStageObjectValues before;
	before.setFrameHandle(m_frameHandle);
	before.setObjectHandle(m_objHandle);
	before.setXsheetHandle(m_xshHandle);
	before.add(m_actionId);
	bool modifyConnectedActionId = false;
	if (m_scaleType != eNone) {
		modifyConnectedActionId = true;
		if (m_actionId == TStageObject::T_ScaleX)
			before.add(TStageObject::T_ScaleY);
		else if (m_actionId == TStageObject::T_ScaleY)
			before.add(TStageObject::T_ScaleX);
		else
			modifyConnectedActionId = false;
	}
	if (m_isGlobalKeyframe) {
		before.add(TStageObject::T_Angle);
		before.add(TStageObject::T_X);
		before.add(TStageObject::T_Y);
		before.add(TStageObject::T_Z);
		before.add(TStageObject::T_SO);
		before.add(TStageObject::T_ScaleX);
		before.add(TStageObject::T_ScaleY);
		before.add(TStageObject::T_Scale);
		before.add(TStageObject::T_Path);
		before.add(TStageObject::T_ShearX);
		before.add(TStageObject::T_ShearY);
	}
	before.updateValues();
	TStageObjectValues after;
	after = before;
	double v = fld->getValue(TMeasuredValue::MainUnit);
	if (modifyConnectedActionId) {
		double oldv1 = after.getValue(0);
		double oldv2 = after.getValue(1);
		double newV;
		if (m_scaleType == eAR)
			newV = (v / oldv1) * oldv2;
		else
			newV = (v == 0) ? 10000 : (1 / v) * (oldv1 * oldv2);
		after.setValues(v, newV);
	} else
		after.setValue(v);
	after.applyValues();

	TTool::Viewer *viewer = m_tool->getViewer();
	if (viewer)
		m_tool->invalidate();
	setCursorPosition(0);

	UndoStageObjectMove *undo = new UndoStageObjectMove(before, after);
	undo->setObjectHandle(m_objHandle);
	TUndoManager::manager()->add(undo);
	TUndoManager::manager()->endBlock();
	m_objHandle->notifyObjectIdChanged(false);
}

//-----------------------------------------------------------------------------

void PegbarChannelField::updateStatus()
{
	TXsheet *xsh = m_tool->getXsheet();
	int frame = m_tool->getFrame();
	TStageObjectId objId = m_tool->getObjectId();
	if (m_actionId == TStageObject::T_Z)
		setMeasure(objId.isCamera() ? "zdepth.cam" : "zdepth");

	double v = xsh->getStageObject(objId)->getParam(m_actionId, frame);

	if (getValue() == v)
		return;
	setValue(v);
	setCursorPosition(0);
}

//-----------------------------------------------------------------------------

void PegbarChannelField::onScaleTypeChanged(int type)
{
	m_scaleType = (ScaleType)type;
}

//=============================================================================

PegbarCenterField::PegbarCenterField(TTool *tool, int index, QString name, TObjectHandle *objHandle, TXsheetHandle *xshHandle,
									 QWidget *parent)
	: MeasuredValueField(parent, name), ToolOptionControl(tool, ""), m_index(index), m_objHandle(objHandle), m_xshHandle(xshHandle)
{
	TStageObjectId objId = m_tool->getObjectId();
	setMeasure(m_index == 0 ? "length.x" : "length.y");
	connect(this, SIGNAL(measuredValueChanged(TMeasuredValue *)), SLOT(onChange(TMeasuredValue *)));
	updateStatus();
}

//-----------------------------------------------------------------------------

void PegbarCenterField::onChange(TMeasuredValue *fld)
{
	if (!m_tool->isEnabled())
		return;
	TXsheet *xsh = m_tool->getXsheet();
	int frame = m_tool->getFrame();
	TStageObjectId objId = m_tool->getObjectId();

	TStageObject *obj = xsh->getStageObject(objId);

	double v = fld->getValue(TMeasuredValue::MainUnit);
	TPointD center = obj->getCenter(frame);
	TPointD oldCenter = center;
	if (m_index == 0)
		center.x = v;
	else
		center.y = v;
	obj->setCenter(frame, center);
	m_tool->invalidate();

	UndoStageObjectCenterMove *undo = new UndoStageObjectCenterMove(objId, frame, oldCenter, center);
	undo->setObjectHandle(m_objHandle);
	undo->setXsheetHandle(m_xshHandle);
	TUndoManager::manager()->add(undo);

	m_objHandle->notifyObjectIdChanged(false);
}

//-----------------------------------------------------------------------------

void PegbarCenterField::updateStatus()
{
	TXsheet *xsh = m_tool->getXsheet();
	int frame = m_tool->getFrame();
	TStageObjectId objId = m_tool->getObjectId();
	TStageObject *obj = xsh->getStageObject(objId);
	TPointD center = obj->getCenter(frame);

	double v = m_index == 0 ? center.x : center.y;
	if (getValue() == v)
		return;
	setValue(v);
}

//=============================================================================

NoScaleField::NoScaleField(TTool *tool, QString name)
	: MeasuredValueField(0, name), ToolOptionControl(tool, "")
{
	TStageObjectId objId = m_tool->getObjectId();
	setMeasure("zdepth");
	connect(this, SIGNAL(measuredValueChanged(TMeasuredValue *)),
			SLOT(onChange(TMeasuredValue *)));
	updateStatus();
}

//-----------------------------------------------------------------------------

void NoScaleField::onChange(TMeasuredValue *fld)
{
	if (!m_tool->isEnabled())
		return;
	TXsheet *xsh = m_tool->getXsheet();
	int frame = m_tool->getFrame();
	TStageObjectId objId = m_tool->getObjectId();
	TStageObject *obj = xsh->getStageObject(objId);

	if (m_isGlobalKeyframe)
		xsh->getStageObject(objId)->setKeyframeWithoutUndo(frame);

	double v = fld->getValue(TMeasuredValue::MainUnit);
	obj->setNoScaleZ(v);
	m_tool->invalidate();
}

//-----------------------------------------------------------------------------

void NoScaleField::updateStatus()
{
	TXsheet *xsh = m_tool->getXsheet();
	int frame = m_tool->getFrame();
	TStageObjectId objId = m_tool->getObjectId();
	TStageObject *obj = xsh->getStageObject(objId);

	double v = obj->getNoScaleZ();
	if (getValue() == v)
		return;
	setValue(v);
}

//=============================================================================

PropertyMenuButton::PropertyMenuButton(QWidget *parent, TTool *tool,
									   QList<TBoolProperty *> properties,
									   QIcon icon, QString tooltip)
	: QToolButton(parent), ToolOptionControl(tool, ""), m_properties(properties)
{
	setPopupMode(QToolButton::InstantPopup);
	setIcon(icon);
	setToolTip(tooltip);

	QMenu *menu = new QMenu(tooltip, this);
	if (!tooltip.isEmpty())
		tooltip = tooltip + " ";

	QActionGroup *actiongroup = new QActionGroup(this);
	actiongroup->setExclusive(false);
	int i;
	for (i = 0; i < m_properties.count(); i++) {
		TBoolProperty *prop = m_properties.at(i);
		QString propertyName = prop->getQStringName();
		//Se il tooltip essagnato e' contenuto nel nome della proprieta' lo levo dalla stringa dell'azione
		if (propertyName.contains(tooltip))
			propertyName.remove(tooltip);
		QAction *action = menu->addAction(propertyName);
		action->setCheckable(true);
		action->setChecked(prop->getValue());
		action->setData(QVariant(i));
		actiongroup->addAction(action);
	}
	bool ret = connect(actiongroup, SIGNAL(triggered(QAction *)), SLOT(onActionTriggered(QAction *)));
	assert(ret);

	setMenu(menu);
}

//-----------------------------------------------------------------------------

void PropertyMenuButton::updateStatus()
{
	QMenu *m = menu();
	assert(m);
	QList<QAction *> actionList = m->actions();
	assert(actionList.count() == m_properties.count());

	int i;
	for (i = 0; i < m_properties.count(); i++) {
		TBoolProperty *prop = m_properties.at(i);
		QAction *action = actionList.at(i);
		bool isPropertyLocked = prop->getValue();
		if (action->isChecked() != isPropertyLocked)
			action->setChecked(isPropertyLocked);
	}
}

//-----------------------------------------------------------------------------

void PropertyMenuButton::onActionTriggered(QAction *action)
{
	int currentPropertyIndex = action->data().toInt();
	TBoolProperty *prop = m_properties.at(currentPropertyIndex);
	bool isChecked = action->isChecked();
	if (isChecked == prop->getValue())
		return;
	prop->setValue(isChecked);
	notifyTool();

	emit onPropertyChanged(QString(prop->getName().c_str()));
}

//=============================================================================
//id == 0 Scale X
//id == 0 Scale Y
SelectionScaleField::SelectionScaleField(SelectionTool *tool, int id, QString name)
	: MeasuredValueField(0, name), m_tool(tool), m_id(id)
{
	bool ret = connect(this, SIGNAL(measuredValueChanged(TMeasuredValue *)), SLOT(onChange(TMeasuredValue *)));
	assert(ret);
	setMeasure("scale");
	updateStatus();
}

//-----------------------------------------------------------------------------

bool SelectionScaleField::applyChange()
{
	if (!m_tool || (m_tool->isSelectionEmpty() && !m_tool->isLevelType()))
		return false;
	DragSelectionTool::DragTool *scaleTool = createNewScaleTool(m_tool, 0);
	double p = getValue();
	if (p == 0)
		p = 0.00001;
	DragSelectionTool::FourPoints points = m_tool->getBBox();
	TPointD center = m_tool->getCenter();
	TPointD p0M = points.getPoint(7);
	TPointD p1M = points.getPoint(5);
	TPointD pM1 = points.getPoint(6);
	TPointD pM0 = points.getPoint(4);
	int pointIndex;
	TPointD sign(1, 1);
	TPointD scaleFactor = m_tool->m_deformValues.m_scaleValue;
	TPointD newPos;
	if (m_id == 0) {
		if (p1M == p0M)
			return false;
		pointIndex = 7;
		TPointD v = normalize(p1M - p0M);
		double currentD = tdistance(p1M, p0M);
		double startD = currentD / scaleFactor.x;
		double d = (currentD - startD * p) * tdistance(center, p0M) / currentD;
		newPos = TPointD(p0M.x + d * v.x, p0M.y + d * v.y);
		scaleFactor.x = p;
	} else if (m_id == 1) {
		if (pM1 == pM0)
			return false;
		pointIndex = 4;
		TPointD v = normalize(pM1 - pM0);
		double currentD = tdistance(pM1, pM0);
		double startD = currentD / scaleFactor.y;
		double d = (currentD - startD * p) * tdistance(center, pM0) / currentD;
		newPos = TPointD(pM0.x + d * v.x, pM0.y + d * v.y);
		scaleFactor.y = p;
	}

	m_tool->m_deformValues.m_scaleValue = scaleFactor; // Instruction order is relevant
	scaleTool->transform(pointIndex, newPos);		   // This line invokes GUI update using the
													   // value set above
	if (!m_tool->isLevelType())
		scaleTool->addTransformUndo();
	setCursorPosition(0);
	return true;
}

//-----------------------------------------------------------------------------

void SelectionScaleField::onChange(TMeasuredValue *fld)
{
	if (!m_tool->isEnabled())
		return;
	if (!applyChange())
		return;
	emit valueChange();
}

//-----------------------------------------------------------------------------

void SelectionScaleField::updateStatus()
{
	if (!m_tool || (m_tool->isSelectionEmpty() && !m_tool->isLevelType())) {
		setValue(0);
		setDisabled(true);
		return;
	}
	setDisabled(false);
	if (m_id == 0)
		setValue(m_tool->m_deformValues.m_scaleValue.x);
	else
		setValue(m_tool->m_deformValues.m_scaleValue.y);
	setCursorPosition(0);
}

//=============================================================================

SelectionRotationField::SelectionRotationField(SelectionTool *tool, QString name)
	: MeasuredValueField(0, name), m_tool(tool)
{
	bool ret = connect(this, SIGNAL(measuredValueChanged(TMeasuredValue *)), SLOT(onChange(TMeasuredValue *)));
	assert(ret);
	setMeasure("angle");
	updateStatus();
}

//-----------------------------------------------------------------------------

void SelectionRotationField::onChange(TMeasuredValue *fld)
{
	if (!m_tool || !m_tool->isEnabled() || (m_tool->isSelectionEmpty() && !m_tool->isLevelType()))
		return;

	DragSelectionTool::DragTool *rotationTool = createNewRotationTool(m_tool);

	DragSelectionTool::DeformValues &deformValues = m_tool->m_deformValues;
	double p = getValue();

	TAffine aff = TRotation(m_tool->getCenter(), p - deformValues.m_rotationAngle);

	deformValues.m_rotationAngle = p;								// Instruction order is relevant here
	rotationTool->transform(aff, p - deformValues.m_rotationAngle); //

	if (!m_tool->isLevelType())
		rotationTool->addTransformUndo();

	setCursorPosition(0);
}

//-----------------------------------------------------------------------------

void SelectionRotationField::updateStatus()
{
	if (!m_tool || (m_tool->isSelectionEmpty() && !m_tool->isLevelType())) {
		setValue(0);
		setDisabled(true);
		return;
	}
	setDisabled(false);
	setValue(m_tool->m_deformValues.m_rotationAngle);
	setCursorPosition(0);
}

//=============================================================================
//id == 0 Move X
//id == 0 Move Y
SelectionMoveField::SelectionMoveField(SelectionTool *tool, int id, QString name)
	: MeasuredValueField(0, name), m_tool(tool), m_id(id)
{
	bool ret = connect(this, SIGNAL(measuredValueChanged(TMeasuredValue *)), SLOT(onChange(TMeasuredValue *)));
	assert(ret);
	if (m_id == 0)
		setMeasure("length.x");
	else
		setMeasure("length.y");
	updateStatus();
}

//-----------------------------------------------------------------------------

void SelectionMoveField::onChange(TMeasuredValue *fld)
{
	if (!m_tool || !m_tool->isEnabled() || (m_tool->isSelectionEmpty() && !m_tool->isLevelType()))
		return;

	DragSelectionTool::DragTool *moveTool = createNewMoveSelectionTool(m_tool);

	double p = getValue() * Stage::inch;
	TPointD delta = (m_id == 0) ? TPointD(p, 1) : TPointD(1, p),
			oldMove = Stage::inch * m_tool->m_deformValues.m_moveValue,
			oldDelta = (m_id == 0) ? TPointD(oldMove.x, 1) : TPointD(1, oldMove.y),
			newMove = (m_id == 0) ? TPointD(delta.x, oldMove.y) : TPointD(oldMove.x, delta.y);

	TAffine aff = TTranslation(-oldDelta) * TTranslation(delta);

	m_tool->m_deformValues.m_moveValue = 1 / Stage::inch * newMove; // Instruction order relevant here
	moveTool->transform(aff);										//

	if (!m_tool->isLevelType())
		moveTool->addTransformUndo();

	setCursorPosition(0);
}

//-----------------------------------------------------------------------------

void SelectionMoveField::updateStatus()
{
	if (!m_tool || (m_tool->isSelectionEmpty() && !m_tool->isLevelType())) {
		setValue(0);
		setDisabled(true);
		return;
	}
	setDisabled(false);

	if (m_id == 0)
		setValue(m_tool->m_deformValues.m_moveValue.x);
	else
		setValue(m_tool->m_deformValues.m_moveValue.y);

	setCursorPosition(0);
}

//=============================================================================

ThickChangeField::ThickChangeField(SelectionTool *tool, QString name)
	: MeasuredValueField(0, name), m_tool(tool)
{
	bool ret = connect(this, SIGNAL(measuredValueChanged(TMeasuredValue *)), SLOT(onChange(TMeasuredValue *)));
	assert(ret);
	setMeasure("");
	updateStatus();
}

//-----------------------------------------------------------------------------

void ThickChangeField::onChange(TMeasuredValue *fld)
{
	if (!m_tool || (m_tool->isSelectionEmpty() && !m_tool->isLevelType()))
		return;

	DragSelectionTool::VectorChangeThicknessTool *changeThickTool = new DragSelectionTool::VectorChangeThicknessTool((VectorSelectionTool *)m_tool);

	TVectorImageP vi = (TVectorImageP)m_tool->getImage(true);

	double p = 0.5 * getValue();
	double newThickness = p - m_tool->m_deformValues.m_maxSelectionThickness;

	changeThickTool->setThicknessChange(newThickness);
	changeThickTool->changeImageThickness(*vi, newThickness);

	//DragSelectionTool::DeformValues deformValues = m_tool->m_deformValues;          // Like when I found it - it's a noop.
	//deformValues.m_maxSelectionThickness = p;                                       // Seems that the actual update is performed inside
	// the above change..() instruction...   >_<
	changeThickTool->addUndo();
	m_tool->computeBBox();
	m_tool->invalidate();
	m_tool->notifyImageChanged(m_tool->getCurrentFid());
}

//-----------------------------------------------------------------------------

void ThickChangeField::updateStatus()
{
	if (!m_tool || m_tool->m_deformValues.m_isSelectionModified || (m_tool->isSelectionEmpty() && !m_tool->isLevelType())) {
		setValue(0);
		setDisabled(true);
		return;
	}

	setDisabled(false);
	setValue(2 * m_tool->m_deformValues.m_maxSelectionThickness);
	setCursorPosition(0);
}
