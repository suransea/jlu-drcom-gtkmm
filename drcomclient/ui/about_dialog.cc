//
// Created by sea on 12/23/18.
//

#include "about_dialog.h"

namespace drcomclient {

AboutDialog::AboutDialog(BaseObjectType *object, const Glib::RefPtr<Gtk::Builder> &builder)
    : Gtk::Dialog(object) {
}

} //namespace drcomclient