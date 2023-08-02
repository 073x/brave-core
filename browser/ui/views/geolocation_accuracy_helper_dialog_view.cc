/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/geolocation_accuracy_helper_dialog_view.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ui/geolocation/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/geolocation/geolocation_accuracy_utils_win.h"
#endif

namespace brave {

void ShowGeolocationAccuracyHelperDialog(content::WebContents* web_contents,
                                         base::OnceClosure closing_callback) {
  constrained_window::ShowWebModalDialogViews(
      new GeolocationAccuracyHelperDialogView(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()),
          std::move(closing_callback)),
      web_contents);
}

}  // namespace brave

namespace {

constexpr char kLearnMoreURL[] =
    "https://support.microsoft.com/en-us/windows/"
    "windows-location-service-and-privacy-3a8eee0a-5b0b-dc07-eede-2a5ca1c49088";

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}

// Subclass for custom font.
class DontShowAgainCheckbox : public views::Checkbox {
 public:
  METADATA_HEADER(DontShowAgainCheckbox);

  using views::Checkbox::Checkbox;
  ~DontShowAgainCheckbox() override = default;
  DontShowAgainCheckbox(const DontShowAgainCheckbox&) = delete;
  DontShowAgainCheckbox& operator=(const DontShowAgainCheckbox&) = delete;

  void SetFontList(const gfx::FontList& font_list) {
    label()->SetFontList(font_list);
  }
};

BEGIN_METADATA(DontShowAgainCheckbox, views::Checkbox)
END_METADATA

}  // namespace

GeolocationAccuracyHelperDialogView::GeolocationAccuracyHelperDialogView(
    PrefService* prefs,
    base::OnceClosure closing_callback)
    : prefs_(prefs) {
  RegisterWindowClosingCallback(std::move(closing_callback));
  set_should_ignore_snapping(true);
  SetModalType(ui::MODAL_TYPE_CHILD);
  SetShowCloseButton(false);
  SetButtons(ui::DIALOG_BUTTON_OK);

  // Safe to use Unretained() here because this callback is only called before
  // closing this widget.
  SetAcceptCallback(base::BindOnce(
      &GeolocationAccuracyHelperDialogView::OnAccept, base::Unretained(this)));

  constexpr int kNarrowChildSpacing = 8;
  constexpr int kChildSpacing = 16;
  constexpr int kPadding = 24;
  constexpr int kTopPadding = 32;
  constexpr int kBottomPadding = 26;

  SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets::TLBR(kTopPadding, kPadding, kBottomPadding, kPadding)))
      ->set_cross_axis_alignment(views::BoxLayout::CrossAxisAlignment::kStart);

  constexpr int kHeaderFontSize = 15;
  auto* header_label =
      AddChildView(std::make_unique<views::Label>(l10n_util::GetStringUTF16(
          IDS_GEOLOCATION_ACCURACY_HELPER_DLG_HEADER_LABEL)));
  header_label->SetFontList(
      GetFont(kHeaderFontSize, gfx::Font::Weight::SEMIBOLD));
  header_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  std::vector<size_t> offset;
  const std::u16string contents_text_part_one = l10n_util::GetStringUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_CONTENTS_PART_ONE_LABEL);
  const std::u16string contents_text_part_two = l10n_util::GetStringUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_CONTENTS_PART_TWO_LABEL);
  const std::u16string contents_text = l10n_util::GetStringFUTF16(
      IDS_GEOLOCATION_ACCURACY_HELPER_DLG_CONTENTS_LABEL,
      contents_text_part_one, contents_text_part_two, &offset);

  constexpr int kContentsFontSize = 14;
  auto* contents_label = AddChildView(std::make_unique<views::StyledLabel>());
  contents_label->SetText(contents_text);
  views::StyledLabel::RangeStyleInfo part_style;
  part_style.custom_font =
      GetFont(kContentsFontSize, gfx::Font::Weight::SEMIBOLD);
  contents_label->AddStyleRange(
      gfx::Range(offset[0], offset[0] + contents_text_part_one.length()),
      part_style);
  contents_label->AddStyleRange(
      gfx::Range(offset[1], offset[1] + contents_text_part_two.length()),
      part_style);
  contents_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  constexpr int kMaxWidth = 450;
  contents_label->SizeToFit(kMaxWidth);

  views::StyledLabel::RangeStyleInfo default_style;
  default_style.custom_font =
      GetFont(kContentsFontSize, gfx::Font::Weight::NORMAL);
  contents_label->AddStyleRange(gfx::Range(0, offset[0]), default_style);
  contents_label->AddStyleRange(
      gfx::Range(offset[0] + contents_text_part_one.length(), offset[1]),
      default_style);
  contents_label->AddStyleRange(
      gfx::Range(offset[1] + contents_text_part_two.length(),
                 contents_text.length()),
      default_style);
  contents_label->SetProperty(views::kMarginsKey,
                              gfx::Insets::TLBR(kChildSpacing, 0, 0, 0));

  auto* learn_more_link =
      AddChildView(std::make_unique<views::Link>(l10n_util::GetStringUTF16(
          IDS_GEOLOCATION_ACCURACY_HELPER_DLG_LEARN_MORE_LABEL)));
  learn_more_link->SetFontList(
      GetFont(kContentsFontSize, gfx::Font::Weight::NORMAL));
  learn_more_link->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  learn_more_link->SetProperty(views::kMarginsKey,
                               gfx::Insets::TLBR(kNarrowChildSpacing, 0, 0, 0));

  // Safe to use Unretained() here because this link is owned by this class.
  learn_more_link->SetCallback(base::BindRepeating(
      &GeolocationAccuracyHelperDialogView::OnLearnMoreClicked,
      base::Unretained(this)));

  // Using Unretained() is safe as |checkbox| is owned by this class.
  auto* checkbox = AddChildView(std::make_unique<DontShowAgainCheckbox>(
      l10n_util::GetStringUTF16(
          IDS_GEOLOCATION_ACCURACY_HELPER_DLG_DONT_SHOW_AGAIN_LABEL),
      base::BindRepeating(
          &GeolocationAccuracyHelperDialogView::OnCheckboxUpdated,
          base::Unretained(this))));
  checkbox->SetFontList(GetFont(kContentsFontSize, gfx::Font::Weight::NORMAL));
  checkbox->SetProperty(views::kMarginsKey,
                        gfx::Insets::TLBR(kChildSpacing, 0, 0, 0));

  dont_show_again_checkbox_ = checkbox;
}

GeolocationAccuracyHelperDialogView::~GeolocationAccuracyHelperDialogView() =
    default;

void GeolocationAccuracyHelperDialogView::OnCheckboxUpdated() {
  prefs_->SetBoolean(kShowGeolocationAccuracyHelperDialog,
                     !dont_show_again_checkbox_->GetChecked());
}

void GeolocationAccuracyHelperDialogView::OnAccept() {
#if BUILDFLAG(IS_WIN)
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&LaunchLocationServiceSettings));
#endif
}

void GeolocationAccuracyHelperDialogView::OnLearnMoreClicked() {
  // Using active window is fine here as this dialog is tied with active tab.
  if (auto* browser = chrome::FindBrowserWithActiveWindow()) {
    chrome::AddSelectedTabWithURL(browser, GURL(kLearnMoreURL),
                                  ui::PAGE_TRANSITION_AUTO_TOPLEVEL);
  }
}

BEGIN_METADATA(GeolocationAccuracyHelperDialogView, views::DialogDelegateView)
END_METADATA
