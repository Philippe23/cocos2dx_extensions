/*
 * ModalAlert - Customizable popup dialogs/alerts for Cocos2D
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
#include "PSModalAlert.h"

USING_NS_COCOS;

#define kDialogTag 1234
#define kAnimationTime 0.4f
#define kDialogImg @"dialogBox.png"
#define kButtonImg @"dialogButton.png"
#define kFontName @"MarkerFelt-Thin"

// Local function declarations
static void PSModalAlertCloseAlert(
	CCSprite *alertDialog,
	CCLayer *coverLayer,
	CCObject *doneSelectorTarget,
	SEL_func doneSelector);


// class that implements a black colored layer that will cover the whole screen 
// and eats all touches except within the dialog box child
class PSCoverLayer: public CCLayerColor
{
public:
	PSCoverLayer() {}
	virtual ~PSCoverLayer() {}

	virtual bool init()
	{
		if ( !self->CCLayerColor::initWithColor(ccc4(0,0,0,0)) )
			return false;

		self->setIsTouchEnabled(true);

		return true;
	}

	virtual bool ccTouchBegan(CCTouch *touch, CCEvent *event)
	{

		CCPoint touchLocation = this->convertTouchToNodeSpace(touch);
		CCNode *dialogBox = this->getChildByTag(kDialogTag);
		CC_ASSERT(dialogBox);
		CCRect bbox = dialogBox->getBoundingBox();

		// eat all touches outside of dialog box.
		return !CCRect::CCRectContainsPoint(bbox, touchLocation);
	}

	virtual void registerWithTouchDispatcher()
	{
		CCTouchDispater::sharedDispatcher()->addTargetedDelegate(
			this,
			INT_MIN+1,
			true); // swallows touches.
	}
};

// Replaces the use of blocks, since C++ doesn't have those.
class PSCloseAndCallBlock: public CCObject
{
public:
	PSCloseAndCallBlock():
		dialog(NULL),
		coverLayer(NULL),
		selectorTarget(NULL),
		selector(NULL)
	{ }

	virtual ~PSCloseAndCallBlock()
	{
		CC_SAFE_RELEASE_NULL(dialog);
		CC_SAFE_RELEASE_NULL(coverLayer);
		CC_SAFE_RELEASE_NULL(selectorTarget);
		this->selector = NULL;
	}

	virtual bool initWithOptions(
		CCNode *dialog,
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_Func selector)
	{
		// if ( !this->CCObject::init() )
		//	return false;
	
		CC_ASSERT(dialog);
		CC_ASSERT(coverLayer);
		CC_ASSERT(selectorTarget);
		CC_ASSERT(selector);

		this->setDialog(dialog);
		this->setCoverLayer(coverLayer);
		this->setSelectorTarget(selectorTarget);
		this->setSelector(selector);

		return true;
	}

	static PSCloseAndCallBlock* closeAndCallBlockWithOptions(
		CCNode *dialog,
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_func selector)
	{
		PSCloseAndCallBlock *cncb = new PSCloseAndCallBlock();
		if (!cncb)
			return NULL;
		bool success = cncb->initWithOptions(
			dialog,
			coverLayer,
			selectorTarget,
			selector);
		if (success)
			cncb->autorelease();
		else
		{
			delete cncb;
			cncb = NULL;
		}
		return cncb;
	}

	void Execute()
	{
		PSModalAlertCloseAlert(
			this->getDialog(),
			this->getCoverLayer(),
			this->getSelectorTarget(),
			this->getSelector() );
	}

	CC_SYNTHESIZE_RETAIN(CCNode*, dialog, Dialog);
	CC_SYNTHESIZE_RETAIN(CCLayer*, coverLayer, CoverLayer);
	CC_SYNTHESIZE_RETAIN(CCObject*, selectorTarget, SelectorTarget);
	CC_SYNTHESIZE(SEL_Func, selector, Selector);
};

class PSWhenDoneBlock: public CCObject
{
public:
	PSWhenDoneBlock():
		coverLayer(NULL),
		selectorTarget(NULL),
		selector(NULL)
	{ }

	virtual ~PSWhenDoneBlock()
	{
		CC_SAFE_RELEASE_NULL(coverLayer);
		CC_SAFE_RELEASE_NULL(selectorTarget);
		this->selector = NULL;
	}

	virtual bool initWithOptions(
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_Func selector)
	{
		// if ( !this->CCObject::init() )
		//	return false;
	
		CC_ASSERT(coverLayer);
		CC_ASSERT(selectorTarget);
		CC_ASSERT(selector);

		this->setCoverLayer(coverLayer);
		this->setSelectorTarget(selectorTarget);
		this->setSelector(selector);

		return true;
	}

	static PSWhenDoneBlock* whenDone(
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_func selector)
	{
		PSWhenDoneBlock *wdb = new PSWhenDoneBlock();
		if (!wdb)
			return NULL;
		bool success = wdb->initWithOptions(
			coverLayer,
			selectorTarget,
			selector);
		if (success)
			wdb->autorelease();
		else
		{
			delete wdb;
			wdb = NULL;
		}
		return wdb;
	}

	void Execute()
	{
		//[CCCallBlock actionWithBlock:^{
		//     [coverLayer removeFromParentAndCleanup:YES];
		//     if (block) block();
		this->getCoverLayer()->removeFromParent(true);
		
		SEL_Func sel = this->getSelector();
		if (sel)
		{
			CCObject *target = this->getSelectorTarget();
			CC_ASSERT(target);
			target->*selector();
		}
		else
		{
			CC_ASSERT( !this->getSelectorTarget() );
		}
	}

	CC_SYNTHESIZE_RETAIN(CCNode*, dialog, Dialog);
	CC_SYNTHESIZE_RETAIN(CCLayer*, coverLayer, CoverLayer);
	CC_SYNTHESIZE_RETAIN(CCObject*, selectorTarget, SelectorTarget);
	CC_SYNTHESIZE(SEL_Func, selector, Selector);
	
};




void PSModalAlertCloseAlert(
	CCSprite *alertDialog,
	CCLayer *coverLayer,
	CCObject *doneSelectorTarget,
	SEL_func doneSelector)
{
	CC_ASSERT(alertDialog);
	CC_ASSERT(coverLayer);

	// Shrink dialog box
	alertDialog->runAction(
		CCScaleTo::actionWithDuration(kAnimationTime, 0.0f) );

	// in parallel, fadeout and remove cover layer and execute
	// block (note: you can't use CCFadeOut since we don't start at
	// opacity 1!)
	PSWhenDoneBlock *wdb = PSWhenDoneBlock::whenDone(
		coverLayer,
		doneSelectorTarget,
		doneSelector);
	CC_ASSSERT(wdb);
	coverLayer->runAction(
		CCSequence::actions(
			CCFadeTo::actionWithDuration(kAnimationTime, 0.0f),
			CCCallFunc::action(
				wdb,
				PSWhenDoneBlock::Execute)
		) );
}



void PSModalAlert::ShowAlert(
	char const *message,
	CCLayer *layer,
	char const *opt1,
	CCObject *opt1SelectorTarget,
	SEL_Func opt1Selector,
	char const *opt2,
	CCObject *opt2SelectorTarget,
	SEL_Func opt2Selector)
{
	CC_ASSERT(message);
	CC_ASSERT(layer);

	// Create the cover layer that "hides" the current application.
	CCLayerColor *coverLayer = PSCoverLayer::node();
	CC_ASSERT(coverLayer);

	// put to the very top to block application touches.
	layer->addChild(coverLayer, INT_MAX);

	// Smooth fade-in to dim with semi-transparency.
	coverLayer->runAction(
		CCFadeTo::actionWithDuration(kAnimationTime, 80) );

	// Open the dialog
	CCSprite *dialog = CCSprite::spriteWithFile(kDialogImg);
	CC_ASSERT(dialog);

	CCSize const & sz = coverLayer->getContentSize();
	CCPoint pos(sz.width, sz.height);

	dialog->setTag(kDialogTag);
	dialog->setPosition( ccpMul(pos, 0.5f) );
	dialog->setOpacity(220); // Make it a bit transparent for a cooler look.

	// Add the alert text.
	CCSize const & dialogSize = dialog->getContentSize();
	CCSize const msgSize(
		dialogSize.width * 0.9f,
		dialogSize.height * 0.55f);
	float const fontSize = UI_USER_INTERFACE_IDIOM() == IPAD ? 42 : 30;

	CCLabelTTF *dialogMsg = CCLabelTTF::labelWithString(
		message,
		msgSize,
		CCTextAlignmentCenter,
		kFontName,
		fontSize);
	CC_ASSERT(dialogMsg);
	// dialogMsg->setAnchorPoint(CCPointZero);
	pos = ccp(dialogSize.width, dialogSize.height);
	pos = ccpMul(pos, 0.5f);
	dialogMsg->setPosition(pos);
	dialogMsg->setColor(ccBLACK);
	dialog->addChild(dialogMsg);

	// add one or two buttons, as needed
	CCMenuItemSprite *opt1Button = CCMenuItemSprite::itemFromNormalSprite(
		CCSprite::spriteWithFile(kButtonImg),
		CCSprite::spriteWithFile(kButtonImg),
	CC_ASSERT(opt1Button);
	pos.x = dialog->getTextureRect().size.width;
	if (opt2)
		pos.x *= 0.27f;
	else pos.x *= 0.5f;
	pos.y = opt1Button->getContentSize().height * 0.8f;
	opt1Button->setPosition(pos);

	CCLabelTTF *opt1Label = CCLabelTTF::labelWithString(
		opt1,
		op1Button->getContentSize(),
		CCTextAlignmentCenter,
		kFontName,
		fontSize);
	opt1Label->setAnchorPoint( ccp(0.0f, 0.1f) );
	opt1Label->setColor(ccBLACK);
	opt1Button->addChild(opt1Label);

	// Create second button if requested.
	CCMenuItemSprite *opt2Button = NULL;
	if (opt2)
	{
		opt2Button = CCMenuItemSprite::itemFromNormalSprite(
			CCSprite::spriteWithFile(kButtonImg),
			CCSprite::spriteWithFile(kButtonImg),
		CC_ASSERT(opt2Button);
		pos.x = dialog->getTextureRect().size.width * 0.73f;
		pos.y = opt1Button->getContentSize().height * 0.8f;
		opt2Button->setPosition(pos);

		CCLabelTTF *opt2Label = CCLabelTTF::labelWithString(
			opt2,
			opt2Button->getContentSize(),
			CCTextAlignmentCenter,
			kFontName,
			fontSize);
		CC_ASSERT(opt2Label);
		opt2Label->setAnchorPoint( ccp(0.0f, 0.1f) );
		opt2Label->setColor(ccBLACK);
		opt2Button->addChild(opt2Label);
	}

	CCMenu *menu = CCMenu::menuWithItems(
		opt1Button,
		opt2Button,
		NULL);
	CC_ASSERT(menu);
	menu->setPosition(CCPointZero);

	dialog->addChild(menu);
	coverLayer->addChild(dialog);

	// Open the dialog with a nice popup-effect
	dialog->setScale(0.0f);
	dialog->runAction(
		CCEaseBackOut::actionWithAction(
			CCScaleTo::actionWithDuration(
				kAnimationTime,
				1.0f)
		) );
}



void PSModalAlert::AskQuestionOnLayer(
	char const * question,
	CCLayer *layer,
	CCObject *yesSelectorTarget,
	SEL_func yesSelector,
	CCObject *noSelectorTarget,
	SEL_Func noSelector)
{
	PSModalAlert::ShowAlert(
		question,
		layer,
		"Yes", yesSelectorTarget, yesSelector,
		"No", noSelectorTarget, noSelector);
}

void PSModalAlert::ConfirmQuestionOnLayer(
	char const * question
	CCLayer *layer,
	CCObject *okSelectorTarget,
	SEL_Func okSelector,
	CCObject *cancelSelectorTarget,
	SEL_Func cancelSelector)
{
	PSModalAlert::ShowAlert(
		question,
		layer,
		"Okay", okSelectorTarget, okSelector,
		"Cancel", cancelSelectorTarget, cancelSelector);
}

void PSModalAlert::TellStatementOnLayer(
	char const * statement,
	CCLayer *layer,
	CCObject *selectorTarget,
	SEL_Func selector)
{
	PSModalAlert::ShowAlert(
		statement,
		layer,
		"Okay", selectorTarget, selector,
		NULL, NULL, NULL);
}
