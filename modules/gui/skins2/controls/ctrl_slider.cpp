/*****************************************************************************
 * ctrl_slider.cpp
 *****************************************************************************
 * Copyright (C) 2003 VideoLAN
 * $Id: ctrl_slider.cpp,v 1.1 2004/01/03 23:31:33 asmax Exp $
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
 *          Olivier Teuli�re <ipkiss@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#include "ctrl_slider.hpp"
#include "../events/evt_enter.hpp"
#include "../events/evt_mouse.hpp"
#include "../events/evt_scroll.hpp"
#include "../src/generic_bitmap.hpp"
#include "../src/generic_window.hpp"
#include "../src/os_factory.hpp"
#include "../src/os_graphics.hpp"
#include "../utils/position.hpp"
#include "../utils/var_percent.hpp"


#define RANGE 40
#define SCROLL_STEP 0.05


CtrlSliderCursor::CtrlSliderCursor( intf_thread_t *pIntf,
                                    const GenericBitmap &rBmpUp,
                                    const GenericBitmap &rBmpOver,
                                    const GenericBitmap &rBmpDown,
                                    const Bezier &rCurve,
                                    VarPercent &rVariable,
                                    VarBool *pVisible,
                                    const UString &rTooltip,
                                    const UString &rHelp ):
    CtrlGeneric( pIntf, rHelp ), m_fsm( pIntf ), m_rVariable( rVariable ),
    m_pVisible( pVisible ), m_tooltip( rTooltip ),
    m_width( rCurve.getWidth() ), m_height( rCurve.getHeight() ),
    m_cmdOverDown( this, &transOverDown ),
    m_cmdDownOver( this, &transDownOver ), m_cmdOverUp( this, &transOverUp ),
    m_cmdUpOver( this, &transUpOver ), m_cmdMove( this, &transMove ),
    m_cmdScroll( this, &transScroll ),
    m_lastPercentage( 0 ), m_xOffset( 0 ), m_yOffset( 0 ),
    m_pEvt( NULL ), m_curve( rCurve )
{
    // Build the images of the cursor
    OSFactory *pOsFactory = OSFactory::instance( getIntf() );
    m_pImgUp = pOsFactory->createOSGraphics( rBmpUp.getWidth(),
                                             rBmpUp.getHeight() );
    m_pImgUp->drawBitmap( rBmpUp, 0, 0 );
    m_pImgDown = pOsFactory->createOSGraphics( rBmpDown.getWidth(),
                                               rBmpDown.getHeight() );
    m_pImgDown->drawBitmap( rBmpDown, 0, 0 );
    m_pImgOver = pOsFactory->createOSGraphics( rBmpOver.getWidth(),
                                               rBmpOver.getHeight() );
    m_pImgOver->drawBitmap( rBmpOver, 0, 0 );

    // States
    m_fsm.addState( "up" );
    m_fsm.addState( "over" );
    m_fsm.addState( "down" );

    // Transitions
    m_fsm.addTransition( "over", "mouse:left:down", "down",
                         &m_cmdOverDown );
    m_fsm.addTransition( "down", "mouse:left:up", "over",
                         &m_cmdDownOver );
    m_fsm.addTransition( "over", "leave", "up", &m_cmdOverUp );
    m_fsm.addTransition( "up", "enter", "over", &m_cmdUpOver );
    m_fsm.addTransition( "down", "motion", "down", &m_cmdMove );
    m_fsm.addTransition( "over", "scroll", "over", &m_cmdScroll );

    // Initial state
    m_fsm.setState( "up" );
    m_pImg = m_pImgUp;

    // Observe the position variable
    m_rVariable.addObserver( this );

    // Observe the visibility variable
    if( m_pVisible )
    {
        m_pVisible->addObserver( this );
    }

    // Initial position of the cursor
    m_lastPercentage = m_rVariable.get();
}


CtrlSliderCursor::~CtrlSliderCursor()
{
    m_rVariable.delObserver( this );
    if( m_pVisible )
    {
        m_pVisible->delObserver( this );
    }
    SKINS_DELETE( m_pImgUp );
    SKINS_DELETE( m_pImgDown );
    SKINS_DELETE( m_pImgOver );
}


void CtrlSliderCursor::handleEvent( EvtGeneric &rEvent )
{
    // Save the event to use it in callbacks
    m_pEvt = &rEvent;

    m_fsm.handleTransition( rEvent.getAsString() );
}


bool CtrlSliderCursor::mouseOver( int x, int y ) const
{
    if( m_pImg )
    {
        // Compute the position of the cursor
        int xPos, yPos;
        m_curve.getPoint( m_rVariable.get(), xPos, yPos );

        // Compute the resize factors
        double factorX = 0, factorY = 0;
        getResizeFactors( factorX, factorY );
        xPos = (int)(xPos * factorX);
        yPos = (int)(yPos * factorY);

        return m_pImg->hit( x - xPos + m_pImg->getWidth() / 2,
                            y - yPos + m_pImg->getHeight() / 2 );
    }
    else
    {
        return false;
    }
}


void CtrlSliderCursor::draw( OSGraphics &rImage, int xDest, int yDest )
{
    if( m_pImg && (!m_pVisible || m_pVisible->get()) )
    {
        // Compute the position of the cursor
        int xPos, yPos;
        m_curve.getPoint( m_rVariable.get(), xPos, yPos );

        // Compute the resize factors
        double factorX = 0, factorY = 0;
        getResizeFactors( factorX, factorY );
        xPos = (int)(xPos * factorX);
        yPos = (int)(yPos * factorY);

        // Draw the current image
        rImage.drawGraphics( *m_pImg, 0, 0,
                             xDest + xPos - m_pImg->getWidth() / 2,
                             yDest + yPos - m_pImg->getHeight() / 2 );
    }
}


void CtrlSliderCursor::onUpdate( Subject<VarPercent> &rVariable )
{
    // The position has changed
    notifyLayout();
}


void CtrlSliderCursor::onUpdate( Subject<VarBool> &rVariable )
{
    // The visibility variable has changed
    notifyLayout();
}


void CtrlSliderCursor::transOverDown( SkinObject *pCtrl )
{
    CtrlSliderCursor *pThis = (CtrlSliderCursor*)pCtrl;
    EvtMouse *pEvtMouse = (EvtMouse*)pThis->m_pEvt;

    // Compute the resize factors
    double factorX = 0, factorY = 0;
    pThis->getResizeFactors( factorX, factorY );

    // Compute the offset
    int tempX, tempY;
    pThis->m_curve.getPoint( pThis->m_rVariable.get(), tempX, tempY );
    pThis->m_xOffset = pEvtMouse->getXPos() - (int)(tempX * factorX);
    pThis->m_yOffset = pEvtMouse->getYPos() - (int)(tempY * factorY);

    pThis->captureMouse();
    pThis->m_pImg = pThis->m_pImgDown;
    pThis->notifyLayout();
}


void CtrlSliderCursor::transDownOver( SkinObject *pCtrl )
{
    CtrlSliderCursor *pThis = (CtrlSliderCursor*)pCtrl;

    // Save the position
    pThis->m_lastPercentage = pThis->m_rVariable.get();

    pThis->releaseMouse();
    pThis->m_pImg = pThis->m_pImgUp;
    pThis->notifyLayout();
}


void CtrlSliderCursor::transUpOver( SkinObject *pCtrl )
{
    CtrlSliderCursor *pThis = (CtrlSliderCursor*)pCtrl;

    pThis->m_pImg = pThis->m_pImgOver;
    pThis->notifyLayout();
}


void CtrlSliderCursor::transOverUp( SkinObject *pCtrl )
{
    CtrlSliderCursor *pThis = (CtrlSliderCursor*)pCtrl;

    pThis->m_pImg = pThis->m_pImgUp;
    pThis->notifyLayout();
}


void CtrlSliderCursor::transMove( SkinObject *pCtrl )
{
    CtrlSliderCursor *pThis = (CtrlSliderCursor*)pCtrl;
    EvtMouse *pEvtMouse = (EvtMouse*)pThis->m_pEvt;

    // Get the position of the control
    const Position *pPos = pThis->getPosition();

    // Compute the resize factors
    double factorX = 0, factorY = 0;
    pThis->getResizeFactors( factorX, factorY );

    // XXX: This could be optimized a little bit
    if( pThis->m_curve.getMinDist(
        (int)((pEvtMouse->getXPos() - pPos->getLeft()) / factorX),
        (int)((pEvtMouse->getYPos() - pPos->getTop()) / factorY) ) < RANGE )
    {
        double percentage = pThis->m_curve.getNearestPercent(
            (int)((pEvtMouse->getXPos() - pThis->m_xOffset) / factorX),
            (int)((pEvtMouse->getYPos() - pThis->m_yOffset) / factorY) );
        pThis->m_rVariable.set( percentage );
    }
    else
    {
        pThis->m_rVariable.set( pThis->m_lastPercentage );
    }
}

void CtrlSliderCursor::transScroll( SkinObject *pCtrl )
{
    CtrlSliderCursor *pThis = (CtrlSliderCursor*)pCtrl;
    EvtScroll *pEvtScroll = (EvtScroll*)pThis->m_pEvt;

    int direction = pEvtScroll->getDirection();

    double percentage = pThis->m_rVariable.get();
    if( direction == EvtScroll::kUp )
    {
        percentage += SCROLL_STEP;
    }
    else
    {
        percentage -= SCROLL_STEP;
    }

    pThis->m_rVariable.set( percentage );
}


void CtrlSliderCursor::getResizeFactors( double &rFactorX,
                                         double &rFactorY ) const
{
    // Get the position of the control
    const Position *pPos = getPosition();

    rFactorX = 1.0;
    rFactorY = 1.0;

    // Compute the resize factors
    if( m_width > 0 )
    {
        rFactorX = (double)pPos->getWidth() / (double)m_width;
    }
    if( m_height > 0 )
    {
        rFactorY = (double)pPos->getHeight() / (double)m_height;
    }
}



CtrlSliderBg::CtrlSliderBg( intf_thread_t *pIntf, CtrlSliderCursor &rCursor,
                            const Bezier &rCurve, VarPercent &rVariable,
                            int thickness, VarBool *pVisible,
                            const UString &rHelp ):
    CtrlGeneric( pIntf, rHelp ), m_rCursor( rCursor ), m_rVariable( rVariable ),
    m_thickness( thickness ), m_pVisible( pVisible ), m_curve( rCurve ),
    m_width( rCurve.getWidth() ), m_height( rCurve.getHeight() )
{
}


bool CtrlSliderBg::mouseOver( int x, int y ) const
{
    if( m_pVisible && !m_pVisible->get() )
    {
        return false;
    }

    // Compute the resize factors
    double factorX = 0, factorY = 1.0;
    getResizeFactors( factorX, factorY );

    return (m_curve.getMinDist( (int)(x / factorY),
                                (int)(y / factorY) ) < m_thickness );
}


void CtrlSliderBg::handleEvent( EvtGeneric &rEvent )
{
    if( rEvent.getAsString().find( "mouse:left:down" ) != string::npos )
    {
        // Compute the resize factors
        double factorX = 0, factorY = 1.0;
        getResizeFactors( factorX, factorY );

        // Get the position of the control
        const Position *pPos = getPosition();

        // Get the value corresponding to the position of the mouse
        EvtMouse &rEvtMouse = (EvtMouse&)rEvent;
        int x = rEvtMouse.getXPos();
        int y = rEvtMouse.getYPos();
        m_rVariable.set( m_curve.getNearestPercent(
                            (int)((x - pPos->getLeft()) / factorX),
                            (int)((y - pPos->getTop()) / factorY) ) );

        // Forward the clic to the cursor
        EvtMouse evt( getIntf(), x, y, EvtMouse::kLeft, EvtMouse::kDown );
        GenericWindow *pWin = getWindow();
        if( pWin )
        {
            EvtEnter evtEnter( getIntf() );
            // XXX It was not supposed to be implemented like that !!
            pWin->forwardEvent( evtEnter, m_rCursor );
            pWin->forwardEvent( evt, m_rCursor );
        }
    }
    else if( rEvent.getAsString().find( "scroll" ) != string::npos )
    {
        int direction = ((EvtScroll&)rEvent).getDirection();

        double percentage = m_rVariable.get();
        if( direction == EvtScroll::kUp )
        {
            percentage += SCROLL_STEP;
        }
        else
        {
            percentage -= SCROLL_STEP;
        }

        m_rVariable.set( percentage );
    }
}


void CtrlSliderBg::onUpdate( Subject<VarBool> &rVariable )
{
    // The visibility variable has changed
    notifyLayout();
}


void CtrlSliderBg::getResizeFactors( double &rFactorX, double &rFactorY ) const
{
    // Get the position of the control
    const Position *pPos = getPosition();

    rFactorX = 1.0;
    rFactorY = 1.0;

    // Compute the resize factors
    if( m_width > 0 )
    {
        rFactorX = (double)pPos->getWidth() / (double)m_width;
    }
    if( m_height > 0 )
    {
        rFactorY = (double)pPos->getHeight() / (double)m_height;
    }
}

