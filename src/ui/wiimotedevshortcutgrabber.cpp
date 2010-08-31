/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ui/wiimotedevshortcutgrabber.h"
#include "ui_wiimotedevshortcutgrabber.h"
#include "wiimotedev/consts.h"

WiimotedevShortcutGrabber::WiimotedevShortcutGrabber(quint32 action, QWidget *parent)
 :QDialog(parent),
  pref_action_(action),
  ui_(new Ui_WiimotedevShortcutGrabber),
  config_(qobject_cast<WiimotedevShortcutsConfig*>(parent)),
  wiimotedev_device_(1),
  wiimotedev_buttons_(0),
  remember_wiimote_shifts_(0),
  remember_nunchuk_shifts_(0)
{
  ui_->setupUi(this);

  if (QDBusConnection::systemBus().isConnected()) {
    wiimotedev_iface_.reset(new OrgWiimotedevDeviceEventsInterface(
        WIIMOTEDEV_DBUS_SERVICE_NAME,
        WIIMOTEDEV_DBUS_EVENTS_OBJECT,
        QDBusConnection::systemBus(),
        this));

    connect(wiimotedev_iface_.get(), SIGNAL(dbusWiimoteGeneralButtons(uint,qulonglong)),
            this, SLOT(DbusWiimoteGeneralButtons(uint,qulonglong)));
  }

  shortcut.action = 0;
  shortcut.button = 0;
  shortcut.object = 0;

  foreach (const QString& name, config_->text_actions_.values())
    ui_->comboBox->addItem(name);

  ui_->comboBox->setCurrentIndex(pref_action_);
  ui_->keep_label->setVisible(false);

  connect(ui_->remember_shifts, SIGNAL(clicked(bool)), this, SLOT(RememberSwingChecked(bool)));
  connect(ui_->buttonBox, SIGNAL(rejected()),  this, SLOT(close()));
  connect(&line_, SIGNAL(frameChanged(int)), this, SLOT(Timeout(int)));

  line_.setFrameRange(4, 0);
  line_.setEasingCurve(QEasingCurve::Linear);
  line_.setDuration(line_.startFrame()*1000);
}

WiimotedevShortcutGrabber::~WiimotedevShortcutGrabber() {
  delete ui_;
}

void WiimotedevShortcutGrabber::Timeout(int secs) {
  if (secs == 0)
    close();

  if (secs == 1)
    ui_->keep_label->setText(QString(tr("Keep buttons for %1 second")).arg(QString::number(secs))); else
    ui_->keep_label->setText(QString(tr("Keep buttons for %1 seconds")).arg(QString::number(secs)));
}

void WiimotedevShortcutGrabber::RememberSwingChecked(bool checked) {
  quint64 buttons = wiimotedev_buttons_;
  line_.stop();
  ui_->keep_label->setVisible(false);


  if (checked) {
    buttons |=  remember_wiimote_shifts_ | remember_nunchuk_shifts_;
    ui_->combo->setText(config_->GetReadableWiiremoteSequence(buttons));
  } else {
    remember_wiimote_shifts_ = 0;
    remember_nunchuk_shifts_ = 0;
    buttons &= ~(WIIMOTE_SHIFT_MASK | NUNCHUK_SHIFT_MASK);
    ui_->combo->setText(config_->GetReadableWiiremoteSequence(buttons));
  }
}


void WiimotedevShortcutGrabber::DbusWiimoteGeneralButtons(uint id, qulonglong value) {
  if (wiimotedev_device_ != id) return;

  quint64 buttons = value & ~(
      WIIMOTE_TILT_MASK |
      NUNCHUK_TILT_MASK |
      WIIMOTE_BTN_SHIFT_SHAKE |
      NUNCHUK_BTN_SHIFT_SHAKE);

  if (ui_->remember_shifts->isChecked()) {
    if (!(buttons & WIIMOTE_SHIFT_MASK)) buttons |= remember_wiimote_shifts_;
    if (!(buttons & NUNCHUK_SHIFT_MASK)) buttons |= remember_nunchuk_shifts_;
  }

  if (wiimotedev_buttons_ == buttons) return;

  remember_wiimote_shifts_ = buttons & WIIMOTE_SHIFT_MASK;
  remember_nunchuk_shifts_ = buttons & NUNCHUK_SHIFT_MASK;

  line_.stop();
  if (buttons) line_.start();

  ui_->keep_label->setVisible(buttons);
  ui_->keep_label->setText(QString(tr("Keep buttons for %1 seconds")).arg(QString::number(line_.startFrame())));
  ui_->combo->setText(config_->GetReadableWiiremoteSequence(buttons));

  wiimotedev_buttons_ = buttons;
}
