//
// Created by sea on 12/23/18.
//

#ifndef JLU_DRCOM_ABOUT_DIALOG_H
#define JLU_DRCOM_ABOUT_DIALOG_H


#include <gtkmm/aboutdialog.h>
#include <gtkmm/builder.h>


namespace drcomclient {

class AboutDialog : public Gtk::Dialog {
public:
  AboutDialog(BaseObjectType *object, const Glib::RefPtr<Gtk::Builder> &builder);

  AboutDialog(const AboutDialog &) = delete;
};

} //namespace drcomclient

#endif //JLU_DRCOM_ABOUT_DIALOG_H
