/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "passport/passport_panel_form.h"

#include "passport/passport_panel_controller.h"
#include "lang/lang_keys.h"
#include "boxes/abstract_box.h"
#include "core/click_handler_types.h"
#include "ui/widgets/shadow.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/scroll_area.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/fade_wrap.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/text_options.h"
#include "ui/special_buttons.h"
#include "styles/style_passport.h"
#include "styles/style_boxes.h"

namespace Passport {

class PanelForm::Row : public Ui::RippleButton {
public:
	Row(
		QWidget *parent,
		const QString &title,
		const QString &description);

	void setReady(bool ready);

protected:
	int resizeGetHeight(int newWidth) override;

	void paintEvent(QPaintEvent *e) override;

private:
	int countAvailableWidth() const;
	int countAvailableWidth(int newWidth) const;

	Text _title;
	Text _description;
	int _titleHeight = 0;
	int _descriptionHeight = 0;
	bool _ready = false;

};

PanelForm::Row::Row(
	QWidget *parent,
	const QString &title,
	const QString &description)
: RippleButton(parent, st::passportRowRipple)
, _title(
	st::semiboldTextStyle,
	title,
	Ui::NameTextOptions(),
	st::boxWideWidth / 2)
, _description(
	st::defaultTextStyle,
	description,
	Ui::NameTextOptions(),
	st::boxWideWidth / 2) {
}

void PanelForm::Row::setReady(bool ready) {
	_ready = ready;
	resizeToWidth(width());
	update();
}

int PanelForm::Row::resizeGetHeight(int newWidth) {
	const auto availableWidth = countAvailableWidth(newWidth);
	_titleHeight = _title.countHeight(availableWidth);
	_descriptionHeight = _description.countHeight(availableWidth);
	const auto result = st::passportRowPadding.top()
		+ _titleHeight
		+ st::passportRowSkip
		+ _descriptionHeight
		+ st::passportRowPadding.bottom();
	return result;
}

int PanelForm::Row::countAvailableWidth(int newWidth) const {
	return newWidth
		- st::passportRowPadding.left()
		- st::passportRowPadding.right()
		- (_ready
			? st::passportRowReadyIcon
			: st::passportRowEmptyIcon).width()
		- st::passportRowIconSkip;
}

int PanelForm::Row::countAvailableWidth() const {
	return countAvailableWidth(width());
}

void PanelForm::Row::paintEvent(QPaintEvent *e) {
	Painter p(this);

	const auto ms = getms();
	paintRipple(p, 0, 0, ms);

	const auto left = st::passportRowPadding.left();
	const auto availableWidth = countAvailableWidth();
	auto top = st::passportRowPadding.top();

	p.setPen(st::passportRowTitleFg);
	_title.drawLeft(p, left, top, availableWidth, width());
	top += _titleHeight + st::passportRowSkip;

	p.setPen(st::passportRowDescriptionFg);
	_description.drawLeft(p, left, top, availableWidth, width());
	top += _descriptionHeight + st::passportRowPadding.bottom();

	const auto &icon = _ready
		? st::passportRowReadyIcon
		: st::passportRowEmptyIcon;
	icon.paint(
		p,
		width() - st::passportRowPadding.right() - icon.width(),
		(height() - icon.height()) / 2,
		width());
}

PanelForm::PanelForm(
	QWidget *parent,
	not_null<PanelController*> controller)
: RpWidget(parent)
, _controller(controller)
, _scroll(this, st::passportPanelScroll)
, _topShadow(this)
, _bottomShadow(this)
, _submit(
		this,
		langFactory(lng_passport_authorize),
		st::passportPanelAuthorize) {
	setupControls();
}

void PanelForm::setupControls() {
	const auto inner = setupContent();

	_submit->addClickHandler([=] {
		_controller->submitForm();
	});

	using namespace rpl::mappers;

	_topShadow->toggleOn(
		_scroll->scrollTopValue() | rpl::map(_1 > 0));
	_bottomShadow->toggleOn(rpl::combine(
		_scroll->scrollTopValue(),
		_scroll->heightValue(),
		inner->heightValue(),
		_1 + _2 < _3));
}

not_null<Ui::RpWidget*> PanelForm::setupContent() {
	const auto bot = _controller->bot();

	const auto inner = _scroll->setOwnedWidget(
		object_ptr<Ui::VerticalLayout>(this));
	_scroll->widthValue(
	) | rpl::start_with_next([=](int width) {
		inner->resizeToWidth(width);
	}, inner->lifetime());

	const auto userpicWrap = inner->add(
		object_ptr<Ui::FixedHeightWidget>(
			inner,
			st::passportFormUserpic.size.height()),
		st::passportFormUserpicPadding);
	_userpic = Ui::AttachParentChild(
		userpicWrap,
		object_ptr<Ui::UserpicButton>(
			userpicWrap,
			bot,
			Ui::UserpicButton::Role::Custom,
			st::passportFormUserpic));
	userpicWrap->widthValue(
	) | rpl::start_with_next([=](int width) {
		_userpic->move((width - _userpic->width()) / 2, _userpic->y());
	}, _userpic->lifetime());

	auto about1 = object_ptr<Ui::FlatLabel>(
		inner,
		lng_passport_request1(lt_bot, App::peerName(bot)),
		Ui::FlatLabel::InitType::Simple,
		st::passportPasswordLabelBold);
	_about1 = about1.data();
	inner->add(
		object_ptr<Ui::IgnoreNaturalWidth>(inner, std::move(about1)),
		st::passportFormAbout1Padding);

	auto about2 = object_ptr<Ui::FlatLabel>(
		inner,
		lang(lng_passport_request2),
		Ui::FlatLabel::InitType::Simple,
		st::passportPasswordLabel);
	_about2 = about2.data();
	inner->add(
		object_ptr<Ui::IgnoreNaturalWidth>(inner, std::move(about2)),
		st::passportFormAbout2Padding);

	inner->add(object_ptr<BoxContentDivider>(
		inner,
		st::passportFormDividerHeight));
	inner->add(
		object_ptr<Ui::FlatLabel>(
			inner,
			lang(lng_passport_header),
			Ui::FlatLabel::InitType::Simple,
			st::passportFormHeader),
		st::passportFormHeaderPadding);

	auto index = 0;
	_controller->fillRows([&](
			QString title,
			QString description,
			bool ready) {
		_rows.push_back(inner->add(object_ptr<Row>(
			this,
			title,
			description)));
		_rows.back()->addClickHandler([=] {
			_controller->editScope(index);
		});
		_rows.back()->setReady(ready);
		++index;
	});
	const auto policyUrl = _controller->privacyPolicyUrl();
	const auto policy = inner->add(
		object_ptr<Ui::FlatLabel>(
			inner,
			(policyUrl.isEmpty()
				? lng_passport_allow(lt_bot, '@' + bot->username)
				: lng_passport_accept_allow(
					lt_policy,
					textcmdLink(
						1,
						lng_passport_policy(lt_bot, App::peerName(bot))),
					lt_bot,
					'@' + bot->username)),
			Ui::FlatLabel::InitType::Rich,
			st::passportFormPolicy),
		st::passportFormPolicyPadding);
	if (!policyUrl.isEmpty()) {
		policy->setLink(1, std::make_shared<UrlClickHandler>(policyUrl));
	}

	return inner;
}

void PanelForm::resizeEvent(QResizeEvent *e) {
	updateControlsGeometry();
}

void PanelForm::updateControlsGeometry() {
	const auto submitTop = height() - _submit->height();
	_scroll->setGeometry(0, 0, width(), submitTop);
	_topShadow->resizeToWidth(width());
	_topShadow->moveToLeft(0, 0);
	_bottomShadow->resizeToWidth(width());
	_bottomShadow->moveToLeft(0, submitTop - st::lineWidth);
	_submit->resizeToWidth(width());
	_submit->moveToLeft(0, submitTop);

	_scroll->updateBars();
}

} // namespace Passport