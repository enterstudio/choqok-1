/*
    This file is part of choqoK, the KDE mono-blogging client

    Copyright (C) 2008-2009 Mehrdad Momeny <mehrdad.momeny@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.


    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see http://www.gnu.org/licenses/

*/
#include "systrayicon.h"
#include "constants.h"
#include "settings.h"
#include <kaction.h>
#include <kactioncollection.h>
#include <QMenu>
#include <QPainter>
#include <KColorScheme>
#include <QProcess>
#include <KNotification>
#include <QTimer>
#include "accountmanager.h"

SysTrayIcon::SysTrayIcon( QWidget* parent ): KSystemTrayIcon( parent )
{
    kDebug();
    unread = 0;
    mainWin = new MainWindow;

    m_defaultIcon = parentWidget()->windowIcon().pixmap( 22 );;

//  setIcon(m_defaultIcon);

    if ( Settings::showMainWinOnStart() ) {
        mainWin->show();
    }
    isIconChanged = false;
    quickWidget = new QuickTwit( mainWin );
    setupActions();

    connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ),
             this, SLOT( sysTrayActivated( QSystemTrayIcon::ActivationReason ) ) );

//  connect(mainWin, SIGNAL(sigStatusUpdated(bool)), this, SLOT(slotStatusUpdated(bool)));

    connect( mainWin, SIGNAL( sigSetUnread( int ) ), this, SLOT( slotSetUnread( int ) ) );

    connect( mainWin, SIGNAL( systemNotify( const QString&, const QString&, const QString& ) ),
             this, SLOT( systemNotify( const QString&, const QString&, const QString& ) ) );

    connect( quickWidget, SIGNAL( sigNotify( const QString&, const  QString&, const  QString& ) ),
             this, SLOT( systemNotify( const QString&, const QString&, const QString& ) ) );
    connect( quickWidget, SIGNAL( sigStatusUpdated( bool ) ), this, SLOT( slotStatusUpdated( bool ) ) );

    connect( qApp, SIGNAL( aboutToQuit() ), this, SLOT( quitApp() ) );
}

SysTrayIcon::~SysTrayIcon()
{
    kDebug();
    AccountManager::self()->deleteLater();
}

void SysTrayIcon::setupActions()
{

    QAction *newTwit = mainWin->actionCollection()->action( "choqok_new_twit" );
    connect( newTwit, SIGNAL( triggered( bool ) ), this, SLOT( postQuickTwit() ) );
    this->contextMenu()->addAction( newTwit );

    contextMenu()->addAction( mainWin->actionCollection()->action( "update_timeline" ) );
    contextMenu()->addSeparator();

    QAction *enableUpdates = mainWin->actionCollection()->action( "choqok_enable_updates" );
    connect( enableUpdates, SIGNAL( toggled( bool ) ), this, SLOT( setTimeLineUpdatesEnabled( bool ) ) );
    contextMenu()->addAction( enableUpdates );
    setTimeLineUpdatesEnabled( enableUpdates->isChecked() );

    contextMenu()->addAction( mainWin->actionCollection()->action( "choqok_enable_notify" ) );
    contextMenu()->addSeparator();
    contextMenu()->addAction( mainWin->actionCollection()->action( "toggle_mainwin" ) );
}

void SysTrayIcon::quitApp()
{
    kDebug();
//  qApp->quit();
    deleteLater();
}

void SysTrayIcon::postQuickTwit()
{
    if ( quickWidget->isVisible() ) {
        quickWidget->hide();
    } else {
        quickWidget->showFocusedOnNewStatusField();
    }
}

void SysTrayIcon::sysTrayActivated( QSystemTrayIcon::ActivationReason reason )
{
    if ( reason == QSystemTrayIcon::Trigger ) {
        switch ( Settings::systrayTriggerType() ) {
        case 0:
            mainWin->toggleMainWindowVisibility();
            break;
        case 1:
            postQuickTwit();
            break;
        };
    }
}

void SysTrayIcon::slotSetUnread( int numOfUnreadStatuses )
{
    kDebug();
//  if (unread == m_unread)
//      return;
    unread += numOfUnreadStatuses;
//     kDebug()<< "unread: " << unread << " numOfUnreadStatuses: " << numOfUnreadStatuses;
//  m_unread=unread;

    this->setToolTip( i18np( "choqoK - 1 unread status", "choqoK - %1 unread statuses",
                             unread > 0 ? unread : 0 ) );

    if ( unread <= 0 ) {
        setIcon( m_defaultIcon );
        isBaseIconChanged = true;
    } else {
        // adapted from KMSystemTray::updateCount()
        int oldWidth = m_defaultIcon.size().width();

        if ( oldWidth == 0 )
            return;

        QString countStr = QString::number( unread );
        QFont f = KGlobalSettings::generalFont();
        f.setBold( true );

        float pointSize = f.pointSizeF();
        QFontMetrics fm( f );
        int w = fm.width( countStr );
        if ( w > ( oldWidth - 2 ) ) {
            pointSize *= float( oldWidth - 2 ) / float( w );
            f.setPointSizeF( pointSize );
        }

        // overlay
        QImage overlayImg = m_defaultIcon.toImage().copy();
        QPainter p( &overlayImg );
        p.setFont( f );
        KColorScheme scheme( QPalette::Active, KColorScheme::View );

        fm = QFontMetrics( f );
        QRect boundingRect = fm.tightBoundingRect( countStr );
        boundingRect.adjust( 0, 0, 0, 2 );
        boundingRect.setHeight( qMin( boundingRect.height(), oldWidth ) );
        boundingRect.moveTo(( oldWidth - boundingRect.width() ) / 2,
                            (( oldWidth - boundingRect.height() ) / 2 ) - 1 );
        p.setOpacity( 0.7 );
        p.setBrush( scheme.background( KColorScheme::LinkBackground ) );
        p.setPen( scheme.background( KColorScheme::LinkBackground ).color() );
        p.drawRoundedRect( boundingRect, 2.0, 2.0 );

        p.setBrush( Qt::NoBrush );
        p.setPen( QColor( 0, 0, 0 ) );
        p.setOpacity( 1.0 );
        p.drawText( overlayImg.rect(), Qt::AlignCenter, countStr );

        setIcon( QPixmap::fromImage( overlayImg ) );
        isBaseIconChanged = true;
    }
}

void SysTrayIcon::systemNotify( const QString &title, const QString &message, const QString &iconUrl )
{
    if ( Settings::notifyType() == 1 ) {//KNotify
        KNotification *notif = new KNotification( "notify", mainWin );
        notif->setText( message );
//         notify->setPixmap(mainWin-);
        notif->setFlags( KNotification::RaiseWidgetOnActivation | KNotification::Persistent );
        notif->sendEvent();
        QTimer::singleShot( Settings::notifyInterval()*1000, notif, SLOT( close() ) );

    } else if ( Settings::notifyType() == 2 ) {//Libnotify!
        QString msg = message;
        msg = msg.replace( "<br/>", "\n" );
        QString libnotifyCmd = QString( "notify-send -t " ) +
                               QString::number( Settings::notifyInterval() * 1000 ) +
                               QString( " -u low -i " + iconUrl + " \"" ) + title +
                               QString( "\" \"" ) + msg + QString( "\"" );
        QProcess::execute( libnotifyCmd );
    }
}

void SysTrayIcon::setTimeLineUpdatesEnabled( bool isEnabled )
{
    if ( isEnabled ) {
        setToolTip( i18n( "choqoK - Hit me to update your status" ) );
        m_defaultIcon = parentWidget()->windowIcon().pixmap( 22 );
    } else {
        slotSetUnread( -unread );
        setToolTip( i18n( "choqoK - Disabled" ) );
        ///Generating new Icon:
        QImage result = m_defaultIcon.toImage();
        for ( int y = 0; y < result.height(); ++y ) {
            for ( int x = 0; x < result.width(); ++x ) {
                int pixel = result.pixel( x, y );
                int gray = qGray( pixel );
                int alpha = qAlpha( pixel );
                result.setPixel( x, y, qRgba( gray, gray, gray, alpha ) );
            }
        }
        m_defaultIcon = QPixmap::fromImage( result );
    }
    setIcon( KIcon( m_defaultIcon ) );
}

void SysTrayIcon::slotStatusUpdated( bool isError )
{
    kDebug();
    if ( !isIconChanged ) {
        prevIcon = icon();
        isIconChanged = true;
    }
    isBaseIconChanged = false;
    if ( isError ) {
        setIcon( KIcon( "dialog-cancel" ) );
    } else {
        setIcon( KIcon( "dialog-ok" ) );
    }
    QTimer::singleShot( 5000, this, SLOT( slotRestoreIcon() ) );
}

void SysTrayIcon::slotRestoreIcon()
{
    if ( !isBaseIconChanged ) {
        setIcon( prevIcon );
    }
    isIconChanged = false;
}

#include "systrayicon.moc"
