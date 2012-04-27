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

USING_NS_CC;

#define kDialogTag 0xdaa999
#define kCoverLayerTag 0xceaea999
#define kAnimationTime 0.4f
#define kDialogImg "dialogBox.png"
#define kButtonImg "dialogButton.png"
#define kFontName "MarkerFelt-Thin"

// Local function declarations
static void PSModalAlertCloseAlert(
	CCNode *alertDialog,
	CCLayer *coverLayer,
	CCObject *doneSelectorTarget,
	SEL_CallFunc doneSelector);

static void PSModalAlertShowAlert(
	char const *message,
	CCLayer *layer,
	char const *opt1,
	CCObject *opt1SelectorTarget,
	SEL_CallFunc opt1Selector,
	char const *opt2,
	CCObject *opt2SelectorTarget,
	SEL_CallFunc opt2Selector);


// class that implements a black colored layer that will cover the whole screen
// and eats all touches except within the dialog box child
class PSCoverLayer: public CCLayerColor
{
public:
	PSCoverLayer() {}
	virtual ~PSCoverLayer() {}

	virtual bool init()
	{
		if ( !this->CCLayerColor::initWithColor(ccc4f(0,0,0,0)) )
			return false;

		this->setIsTouchEnabled(true);

		return true;
	}

	virtual bool ccTouchBegan(CCTouch *touch, CCEvent *event)
	{

		CCPoint touchLocation = this->convertTouchToNodeSpace(touch);
		CCNode *dialogBox = this->getChildByTag(kDialogTag);
		CC_ASSERT(dialogBox);
		CCRect const bbox = dialogBox->boundingBox();

		// eat all touches outside of dialog box.
		return !CCRect::CCRectContainsPoint(bbox, touchLocation);
	}

	virtual void registerWithTouchDispatcher()
	{
		CCTouchDispatcher::sharedDispatcher()->addTargetedDelegate(
			this,
			INT_MIN+1,
			true); // swallows touches.
	}
};

// Replaces the use of blocks, since C++ doesn't have those.
//
// These would be subclassed from CCObject, but we don't get
// the handy auto-reference counting that blocks get.  So my
// hack is to add them as invisible items on the cover layer.
//
// They can't have circular references back to the layer to run
// the code that was in the blocks, so layers and dialogs are
// found and verified by tags (defines near top).
//
// Like CCMenuItem's, they don't retain the selectors they're
// going to call.
class PSCloseAndCallBlock: public CCNode
{
public:
	PSCloseAndCallBlock():
		selectorTarget(NULL),
		selector(NULL)
	{ }

	virtual ~PSCloseAndCallBlock()
	{
		this->selectorTarget = NULL;
		this->selector = NULL;
	}

	virtual bool initWithOptions(
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_CallFunc selector)
	{
		// if ( !this->CCNode::init() )
		//	return false;

		CC_ASSERT(coverLayer);
		CC_ASSERT( (selectorTarget && selector) || (!selectorTarget && !selector) );

		// Note that we automatically glom onto the coverlayer,
		// this is likely all that keeps us around.  We'll
		// go away when it goes away.
		CC_ASSERT(coverLayer->getTag() == kCoverLayerTag);
		coverLayer->addChild(this);

		this->setSelectorTarget(selectorTarget);
		this->setSelector(selector);

		return true;
	}

	static PSCloseAndCallBlock* closeAndCallBlockWithOptions(
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_CallFunc selector)
	{
		PSCloseAndCallBlock *cncb = new PSCloseAndCallBlock();
		if (!cncb)
			return NULL;
		bool success = cncb->initWithOptions(
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

	void Execute(CCNode *menu_item)
	{
		CC_UNUSED_PARAM(menu_item);

		// Parent == CoverLayer & find sibling dialog by tag.
		CCNode *parent = this->getParent();
		CC_ASSERT(parent);
		CC_ASSERT(dynamic_cast<CCLayer*>(parent) != NULL);
		CC_ASSERT(parent->getTag() == kCoverLayerTag);

		CCLayer *coverLayer = static_cast<CCLayer*>(parent);
		CC_ASSERT(coverLayer);
		CCNode *dialog = coverLayer->getChildByTag(kDialogTag);
		CC_ASSERT(dialog);

		PSModalAlertCloseAlert(
			dialog,
			coverLayer,
			this->getSelectorTarget(),
			this->getSelector() );
	}

	CC_SYNTHESIZE(CCObject*, selectorTarget, SelectorTarget);
	CC_SYNTHESIZE(SEL_CallFunc, selector, Selector);
};

// Make sure you read the comment above PSCloseAndCallBlock
class PSWhenDoneBlock: public CCNode
{
public:
	PSWhenDoneBlock():
		selectorTarget(NULL),
		selector(NULL)
	{ }

	virtual ~PSWhenDoneBlock()
	{
		this->selectorTarget = NULL;
		this->selector = NULL;
	}

	virtual bool initWithOptions(
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_CallFunc selector)
	{
		// if ( !this->CCNode::init() )
		//	return false;

		CC_ASSERT(coverLayer);
		CC_ASSERT( (selectorTarget && selector) || (!selectorTarget && !selector) );

		// Note that we automatically glom onto the coverlayer,
		// this is likely all that keeps us around.  We'll
		// go away when it goes away.
		CC_ASSERT(coverLayer->getTag() == kCoverLayerTag);
		coverLayer->addChild(this);

		this->setSelectorTarget(selectorTarget);
		this->setSelector(selector);

		return true;
	}

	static PSWhenDoneBlock* whenDone(
		CCLayer *coverLayer,
		CCObject *selectorTarget,
		SEL_CallFunc selector)
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
		// Code we're replacing:
		//[CCCallBlock actionWithBlock:^{
		//     [coverLayer removeFromParentAndCleanup:YES];
		//     if (block) block();

		// More work than above, coverlayer should be our
		// parent.
		CCNode *parent = this->getParent();
		CC_ASSERT(parent);
		CC_ASSERT(parent->getTag() == kCoverLayerTag);
		CC_ASSERT(dynamic_cast<CCLayer*>(parent) != NULL);
		CCLayer *coverLayer = static_cast<CCLayer*>(parent);
		CC_ASSERT(coverLayer);

		// Retain coverlayer so it doesn't go away on us.
		coverLayer->retain();

		coverLayer->removeFromParentAndCleanup(true);

		SEL_CallFunc sel = this->getSelector();
		if (sel)
		{
			CCObject *target = this->getSelectorTarget();
			CC_ASSERT(target);
			(target->*selector)();
		}
		else
		{
			CC_ASSERT( !this->getSelectorTarget() );
		}

		// Release -- there's a good chance we might be
		// deleted by this next line since being a child
		// of coverLayer is all that keeps us around,
		// so make sure "this" isn't used after it.
		coverLayer->release();
	}

	CC_SYNTHESIZE(CCObject*, selectorTarget, SelectorTarget);
	CC_SYNTHESIZE(SEL_CallFunc, selector, Selector);
};




void PSModalAlertCloseAlert(
	CCNode *alertDialog,
	CCLayer *coverLayer,
	CCObject *doneSelectorTarget,
	SEL_CallFunc doneSelector)
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
	CC_ASSERT(wdb);
	coverLayer->runAction(
		CCSequence::actions(
			CCFadeTo::actionWithDuration(kAnimationTime, 0.0f),
			CCCallFunc::actionWithTarget(
				wdb,
				SEL_CallFunc(&PSWhenDoneBlock::Execute) )
		) );
}



void PSModalAlertShowAlert(
	char const *message,
	CCLayer *layer,
	char const *opt1,
	CCObject *opt1SelectorTarget,
	SEL_CallFunc opt1Selector,
	char const *opt2,
	CCObject *opt2SelectorTarget,
	SEL_CallFunc opt2Selector)
{
	CC_ASSERT(message);
	CC_ASSERT(layer);

	// Create the cover layer that "hides" the current application.
	CCLayerColor *coverLayer = PSCoverLayer::node();
	CC_ASSERT(coverLayer);

	// Tag for later validation.
	coverLayer->setTag(kCoverLayerTag);

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
	dialog->setPosition( ccpMult(pos, 0.5f) );
	dialog->setOpacity(220); // Make it a bit transparent for a cooler look.


	// Add the alert text.
	CCSize const & dialogSize = dialog->getContentSize();
	CCSize const msgSize(
		dialogSize.width * 0.9f,
		dialogSize.height * 0.55f);
	// Trying to replace UI_USER_INTERFACE_IDIOM() == IPAD
	CCSize const winSize = CCDirector::sharedDirector()->getWinSize();
	bool const isiPad = (winSize.width >= 1024.0f) ||
	                    (winSize.height >= 1024.0f);
	float const fontSize =  ( isiPad ? 42 : 30 );

	CCLabelTTF *dialogMsg = CCLabelTTF::labelWithString(
		message,
		msgSize,
		CCTextAlignmentCenter,
		kFontName,
		fontSize);
	CC_ASSERT(dialogMsg);
	// dialogMsg->setAnchorPoint(CCPointZero);
	pos = ccp(dialogSize.width, dialogSize.height);
	pos = ccpMult(pos, 0.5f);
	dialogMsg->setPosition(pos);
	dialogMsg->setColor(ccBLACK);
	dialog->addChild(dialogMsg);

	// add one or two buttons, as needed

	// The following is to replace the Objective-C block
	// in the original.
	PSCloseAndCallBlock *cncb = PSCloseAndCallBlock::closeAndCallBlockWithOptions(
		coverLayer,
		opt1SelectorTarget,
		opt1Selector);
	CC_ASSERT(cncb);

	CCMenuItemSprite *opt1Button = CCMenuItemSprite::itemFromNormalSprite(
		CCSprite::spriteWithFile(kButtonImg),
		CCSprite::spriteWithFile(kButtonImg),
		cncb,
		SEL_MenuHandler(&PSCloseAndCallBlock::Execute) );
	CC_ASSERT(opt1Button);
	pos.x = dialog->getTextureRect().size.width;
	if (opt2)
		pos.x *= 0.27f;
	else pos.x *= 0.5f;
	pos.y = opt1Button->getContentSize().height * 0.8f;
	opt1Button->setPosition(pos);

	CCLabelTTF *opt1Label = CCLabelTTF::labelWithString(
		opt1,
		opt1Button->getContentSize(),
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
		// Replaces Objective-C block in original code.
		cncb = PSCloseAndCallBlock::closeAndCallBlockWithOptions(
			coverLayer,
			opt2SelectorTarget,
			opt2Selector);
		CC_ASSERT(cncb);

		opt2Button = CCMenuItemSprite::itemFromNormalSprite(
			CCSprite::spriteWithFile(kButtonImg),
			CCSprite::spriteWithFile(kButtonImg),
			cncb,
			SEL_MenuHandler(&PSCloseAndCallBlock::Execute) );
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
	SEL_CallFunc yesSelector,
	CCObject *noSelectorTarget,
	SEL_CallFunc noSelector)
{
	PSModalAlertShowAlert(
		question,
		layer,
		"Yes", yesSelectorTarget, yesSelector,
		"No", noSelectorTarget, noSelector);
}

void PSModalAlert::ConfirmQuestionOnLayer(
	char const * question,
	CCLayer *layer,
	CCObject *okSelectorTarget,
	SEL_CallFunc okSelector,
	CCObject *cancelSelectorTarget,
	SEL_CallFunc cancelSelector)
{
	PSModalAlertShowAlert(
		question,
		layer,
		"Okay", okSelectorTarget, okSelector,
		"Cancel", cancelSelectorTarget, cancelSelector);
}

void PSModalAlert::TellStatementOnLayer(
	char const * statement,
	CCLayer *layer,
	CCObject *selectorTarget,
	SEL_CallFunc selector)
{
	PSModalAlertShowAlert(
		statement,
		layer,
		"Okay", selectorTarget, selector,
		NULL, NULL, NULL);
}
