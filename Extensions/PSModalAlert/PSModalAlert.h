/*
 * ModalAlert - Customizable popup dialogs/alerts for Cocos2D
 *
 * For details, visit the Rombos blog:
 * http://rombosblog.wordpress.com/2012/02/28/modal-alerts-for-cocos2d/
 *
 * Copyright (c) 2012 Hans-Juergen Richstein, Rombos
 * http://www.rombos.de
 *
 * C++ version (c) 2012 Philippe Chaintreuil, ParallaxShift
 * http://www.parallaxshift.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "cocos2d.h"


// This is just a class because the Objective-C version was.  A
// PSModalAlert will never be instanciated.
//
// The one other gotcha' is that your selector targets are *not*
// retained, so make sure they stick around until the dialog is
// released.  (Which may be during an autorelease after the dialog
// disappears.)
//
// As the original Objective-C version does, this requires the
// dialogBox{,-hd,-ipad}.png and dialogButton{,-hd,-ipad}.png resources
// to be loadable by CCSprite::spriteWithFile().  You can replace
// them with your own versions that match your color schemes.
//

class PSModalAlert
{
public:
	static void AskQuestionOnLayer(
		char const * question,
		cocos2d::CCLayer *layer,
		cocos2d::CCObject *yesSelectorTarget,
		cocos2d::SEL_CallFunc yesSelector,
		cocos2d::CCObject *noSelectorTarget,
		cocos2d::SEL_CallFunc noSelector);

	static void ConfirmQuestionOnLayer(
		char const * question,
		cocos2d::CCLayer *layer,
		cocos2d::CCObject *okSelectorTarget,
		cocos2d::SEL_CallFunc okSelector,
		cocos2d::CCObject *cancelSelectorTarget,
		cocos2d::SEL_CallFunc cancelSelector);

	static void TellStatementOnLayer(
		char const *statement,
		cocos2d::CCLayer *layer,
		cocos2d::CCObject *okSelectorTarget,
		cocos2d::SEL_CallFunc okSelector);
};
