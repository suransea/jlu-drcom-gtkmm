//
// Created by sea on 12/23/18.
//

#ifndef JLU_DRCOM_PREFERENCES_DIALOG_H
#define JLU_DRCOM_PREFERENCES_DIALOG_H

#include <gtkmm/dialog.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/builder.h>

namespace drcomclient {

class PreferencesDialog : public Gtk::Dialog {
public:
  PreferencesDialog(BaseObjectType *object, const Glib::RefPtr<Gtk::Builder> &builder);

  PreferencesDialog(const PreferencesDialog &) = delete;

  ~PreferencesDialog() override;

protected:
  void init_control();

  void init_signal();

  //Signal handlers:
  void on_save_button_clicked();

  void on_cancel_button_clicked();

  //Member widgets:
  Gtk::CheckButton *auto_min_button_ = nullptr;
  Gtk::Entry *server_ip_entry_ = nullptr;
  Gtk::ComboBoxText *mac_box_ = nullptr;
  Gtk::Button *save_button_ = nullptr;
  Gtk::Button *cancel_button_ = nullptr;
};

} //namespace drcomclient

#endif //JLU_DRCOM_PREFERENCES_DIALOG_H
