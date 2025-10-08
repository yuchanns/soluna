#define SOKOL_IMPL

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#define SOKOL_D3D11

#elif defined(__APPLE__)

#define SOKOL_METAL

#elif defined(__linux__)

#define SOKOL_GLCORE

#else

#error Unsupport platform

#endif

#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdio.h>
#if defined(__APPLE__)
#include <stdbool.h>
#endif
#include "version.h"

#define FRAME_CALLBACK 1
#define CLEANUP_CALLBACK 2
#define EVENT_CALLBACK 3
#define CALLBACK_COUNT 3

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_args.h"
#include "loginfo.h"
#include "appevent.h"

#if defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreText/CoreText.h>
#import <objc/runtime.h>
#import <objc/message.h>
#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imm.h>
#include <windowsx.h>
#include <winnls.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)

#define PLATFORM "windows"

#elif defined(__APPLE__)

#define PLATFORM "macos"

#elif defined(__linux__)

#define PLATFORM "linux"

#else

#define PLATFORM "unknown"

#endif


static void app_event(const sapp_event* ev);

struct app_context {
	lua_State *L;
	lua_State *quitL;
	int (*send_log)(void *ud, unsigned int id, void *data, uint32_t sz);
	void *send_log_ud;
	void *mqueue;
};

static struct app_context *CTX = NULL;

struct soluna_ime_rect_state {
	float x;
	float y;
	float w;
	float h;
	bool valid;
};

#if defined(__APPLE__) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
static struct soluna_ime_rect_state g_soluna_ime_rect = { 0.0f, 0.0f, 0.0f, 0.0f, false };
#endif

#if defined(__APPLE__)
static bool g_soluna_macos_composition = false;
static NSTextField *g_soluna_ime_label = nil;
static NSString *g_soluna_macos_ime_font_name = nil;
static CGFloat g_soluna_macos_ime_font_size = 14.0f;
static NSView *g_soluna_ime_caret = nil;

static NSString *soluna_current_marked_text(NSView *view);
static NSRange soluna_current_selected_range(NSView *view);

static void
soluna_macos_apply_ime_font(void) {
	if (!g_soluna_ime_label) {
		return;
	}
	CGFloat size = g_soluna_macos_ime_font_size > 0.0f ? g_soluna_macos_ime_font_size : 14.0f;
	NSFont *font = nil;
	if (g_soluna_macos_ime_font_name) {
		font = [NSFont fontWithName:g_soluna_macos_ime_font_name size:size];
	}
	if (font == nil) {
		font = [NSFont systemFontOfSize:size];
	}
	[g_soluna_ime_label setFont:font];
}

static void
soluna_macos_set_ime_font(const char *font_name, float height_px) {
	if (g_soluna_macos_ime_font_name) {
		[g_soluna_macos_ime_font_name release];
		g_soluna_macos_ime_font_name = nil;
	}
	if (font_name && font_name[0]) {
		NSString *converted = [[NSString alloc] initWithUTF8String:font_name];
		if (converted) {
			g_soluna_macos_ime_font_name = converted;
		} else {
			[converted release];
		}
	}
	if (height_px > 0.0f) {
		g_soluna_macos_ime_font_size = (CGFloat)height_px;
	} else {
		g_soluna_macos_ime_font_size = 0.0f;
	}
	soluna_macos_apply_ime_font();
}

static NSView *
soluna_macos_ensure_ime_caret(NSView *view) {
	if (g_soluna_ime_caret == nil) {
		g_soluna_ime_caret = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
		[g_soluna_ime_caret setWantsLayer:YES];
		CALayer *layer = [g_soluna_ime_caret layer];
		if (layer) {
			layer.backgroundColor = [[NSColor controlAccentColor] CGColor];
		}
	}
	if (g_soluna_ime_caret.superview != view) {
		[g_soluna_ime_caret removeFromSuperview];
		if (view) {
			NSView *relative = g_soluna_ime_label && g_soluna_ime_label.superview == view ? g_soluna_ime_label : nil;
			[view addSubview:g_soluna_ime_caret positioned:NSWindowAbove relativeTo:relative];
		}
	}
	return g_soluna_ime_caret;
}

static void
soluna_macos_hide_ime_caret(void) {
	if (g_soluna_ime_caret) {
		[g_soluna_ime_caret setHidden:YES];
	}
}

static void
soluna_macos_position_ime_caret(NSView *view, NSTextField *label, NSAttributedString *attr, NSRange selectedRange) {
	if (selectedRange.location == NSNotFound) {
		soluna_macos_hide_ime_caret();
		return;
	}
	NSView *caret = soluna_macos_ensure_ime_caret(view);
	if (selectedRange.length > 0) {
		[caret setHidden:YES];
		return;
	}
	NSUInteger caretIndex = selectedRange.location + selectedRange.length;
	NSUInteger textLength = attr.length;
	if (caretIndex > textLength) {
		caretIndex = textLength;
	}
	NSRect textRect = [[label cell] drawingRectForBounds:label.bounds];
	CGFloat prefixWidth = 0.0f;
	CGFloat lineHeight = 0.0f;
	CTLineRef line = CTLineCreateWithAttributedString((CFAttributedStringRef)attr);
	if (line) {
		if (caretIndex > textLength) {
			caretIndex = textLength;
		}
		double caretOffset = CTLineGetOffsetForStringIndex(line, caretIndex, NULL);
		prefixWidth = (CGFloat)ceil(caretOffset);
		double ascent = 0.0, descent = 0.0, leading = 0.0;
		CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
		lineHeight = (CGFloat)(ascent + descent + leading);
		CFRelease(line);
	}
	NSRect caretFrame;
	caretFrame.size.width = 2.0f;
	if (lineHeight <= 0.0f) {
		NSFont *font = [label font];
		if (font) {
			lineHeight = font.ascender - font.descender;
		}
		if (lineHeight <= 0.0f) {
			lineHeight = g_soluna_macos_ime_font_size > 0.0f ? g_soluna_macos_ime_font_size : 14.0f;
		}
	}
	caretFrame.size.height = MAX(1.0f, lineHeight);
	caretFrame.origin.x = label.frame.origin.x + textRect.origin.x + prefixWidth;
	CGFloat originY = label.frame.origin.y + textRect.origin.y;
	CGFloat verticalAdjust = 0.0f;
	if (textRect.size.height > caretFrame.size.height) {
		verticalAdjust = (textRect.size.height - caretFrame.size.height) * 0.5f;
	}
	caretFrame.origin.y = originY + verticalAdjust;
	[caret setFrame:NSIntegralRect(caretFrame)];
	[caret setHidden:NO];
}

static NSRect
soluna_current_caret_local_rect(NSView *view) {
	NSRect caret = NSMakeRect(0, 0, 1, 1);
	if (g_soluna_ime_rect.valid) {
		CGFloat dpi_scale = sapp_dpi_scale();
		if (dpi_scale <= 0.0f) {
			dpi_scale = 1.0f;
		}
		CGFloat logical_height = (CGFloat)sapp_height() / dpi_scale;
		CGFloat caret_y = logical_height - (g_soluna_ime_rect.y + g_soluna_ime_rect.h);
		caret = NSMakeRect(g_soluna_ime_rect.x, caret_y, g_soluna_ime_rect.w, g_soluna_ime_rect.h);
	}
	return caret;
}

static NSTextField *
soluna_macos_ensure_ime_label(NSView *view) {
	if (g_soluna_ime_label == nil) {
		g_soluna_ime_label = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
		[g_soluna_ime_label setEditable:NO];
		[g_soluna_ime_label setSelectable:NO];
		[g_soluna_ime_label setBezeled:NO];
		[g_soluna_ime_label setDrawsBackground:NO];
		[g_soluna_ime_label setBordered:NO];
		[g_soluna_ime_label setBackgroundColor:[NSColor clearColor]];
		[g_soluna_ime_label setHidden:YES];
		[g_soluna_ime_label setLineBreakMode:NSLineBreakByClipping];
		[g_soluna_ime_label setUsesSingleLineMode:YES];
		[g_soluna_ime_label setTranslatesAutoresizingMaskIntoConstraints:YES];
		soluna_macos_apply_ime_font();
	}
	if (g_soluna_ime_label.superview != view) {
		[g_soluna_ime_label removeFromSuperview];
		if (view) {
			[view addSubview:g_soluna_ime_label];
		}
	}
	soluna_macos_apply_ime_font();
	return g_soluna_ime_label;
}

static void
soluna_macos_hide_ime_label(void) {
	if (g_soluna_ime_label) {
		[g_soluna_ime_label setHidden:YES];
	}
	soluna_macos_hide_ime_caret();
}

static void
soluna_macos_position_ime_label(NSView *view, NSString *text, NSRange selectedRange) {
	if (text == nil || text.length == 0) {
		soluna_macos_hide_ime_label();
		return;
	}
	NSTextField *label = soluna_macos_ensure_ime_label(view);
	if (label == nil) {
		return;
	}
	NSMutableAttributedString *attr = [[[NSMutableAttributedString alloc] initWithString:text] autorelease];
	NSRange fullRange = NSMakeRange(0, attr.length);
	if (fullRange.length > 0) {
		NSFont *labelFont = [label font];
		if (labelFont) {
			[attr addAttribute:NSFontAttributeName value:labelFont range:fullRange];
		}
		[attr addAttribute:NSForegroundColorAttributeName value:[NSColor textColor] range:fullRange];
		[attr addAttribute:NSUnderlineStyleAttributeName value:@(NSUnderlineStyleSingle) range:fullRange];
	}
	if (selectedRange.length > 0 && NSMaxRange(selectedRange) <= attr.length) {
		[attr addAttribute:NSBackgroundColorAttributeName value:[NSColor controlAccentColor] range:selectedRange];
		[attr addAttribute:NSForegroundColorAttributeName value:[NSColor alternateSelectedControlTextColor] range:selectedRange];
	}
	[label setAttributedStringValue:attr];
	NSSize textSize = [[label cell] cellSizeForBounds:NSMakeRect(0, 0, CGFLOAT_MAX, CGFLOAT_MAX)];
	CGFloat padding = 6.0f;
	textSize.width += padding;
	textSize.height += padding;
	NSRect caret = soluna_current_caret_local_rect(view);
	CGFloat baseline = caret.origin.y + caret.size.height;
	CGFloat baselineOffset = [label baselineOffsetFromBottom];
	CGFloat frameOriginY = baseline - baselineOffset - textSize.height;
	NSRect frame = NSMakeRect(caret.origin.x, frameOriginY, textSize.width, textSize.height);
	NSRect bounds = view.bounds;
	if (frame.origin.x < bounds.origin.x) {
		frame.origin.x = bounds.origin.x;
	}
	CGFloat maxX = NSMaxX(bounds);
	if (NSMaxX(frame) > maxX) {
		frame.origin.x = maxX - frame.size.width;
	}
	if (frame.origin.y < bounds.origin.y) {
		frame.origin.y = bounds.origin.y;
	}
	CGFloat maxY = NSMaxY(bounds);
	if (NSMaxY(frame) > maxY) {
		frame.origin.y = maxY - frame.size.height;
	}
	[label setFrame:NSIntegralRect(frame)];
	[label setHidden:NO];
	soluna_macos_position_ime_caret(view, label, attr, selectedRange);
}

static void
soluna_macos_refresh_ime_label(NSView *view) {
	if (g_soluna_ime_label == nil || [g_soluna_ime_label isHidden]) {
		return;
	}
	NSString *text = soluna_current_marked_text(view);
	NSRange range = soluna_current_selected_range(view);
	soluna_macos_position_ime_label(view, text, range);
}
#endif

struct soluna_message {
	const char * type;
	union {
		int p[2];
		uint64_t u64;
	} v;
};

static inline struct soluna_message *
message_create(const char *type, int p1, int p2) {
	struct soluna_message * msg = (struct soluna_message *)malloc(sizeof(*msg));
	msg->type = type;
	msg->v.p[0] = p1;
	msg->v.p[1] = p2;
	return msg;
}

static inline struct soluna_message *
message_create64(const char *type, uint64_t p) {
	struct soluna_message * msg = (struct soluna_message *)malloc(sizeof(*msg));
	msg->type = type;
	msg->v.u64 = p;
	return msg;
}

static inline void
message_release(struct soluna_message *msg) {
	free(msg);
}

#if defined(__APPLE__)
static const void *const kSolunaMarkedTextKey = &kSolunaMarkedTextKey;
static const void *const kSolunaSelectedRangeKey = &kSolunaSelectedRangeKey;
static const void *const kSolunaConsumedFlagKey = &kSolunaConsumedFlagKey;

static uint32_t
soluna_modifiers_from_event(NSEvent *event) {
	NSEventModifierFlags flags = event ? event.modifierFlags : NSEvent.modifierFlags;
	uint32_t mods = 0;
	if (flags & NSEventModifierFlagShift) {
		mods |= SAPP_MODIFIER_SHIFT;
	}
	if (flags & NSEventModifierFlagControl) {
		mods |= SAPP_MODIFIER_CTRL;
	}
	if (flags & NSEventModifierFlagOption) {
		mods |= SAPP_MODIFIER_ALT;
	}
	if (flags & NSEventModifierFlagCommand) {
		mods |= SAPP_MODIFIER_SUPER;
	}
	return mods;
}

static uint32_t
soluna_utf32_from_substring(NSString *substr) {
	if (substr == nil || substr.length == 0) {
		return 0;
	}
	unichar buffer[2] = {0};
	NSUInteger len = substr.length;
	[substr getCharacters:buffer range:NSMakeRange(0, len)];
	if (len >= 2 && buffer[0] >= 0xD800 && buffer[0] <= 0xDBFF && buffer[1] >= 0xDC00 && buffer[1] <= 0xDFFF) {
		uint32_t high = buffer[0] - 0xD800;
		uint32_t low = buffer[1] - 0xDC00;
		return (high << 10) + low + 0x10000;
	}
	return buffer[0];
}

static NSString *
soluna_plain_string(id string) {
	if ([string isKindOfClass:[NSAttributedString class]]) {
		return [(NSAttributedString *)string string];
	}
	if ([string isKindOfClass:[NSString class]]) {
		return (NSString *)string;
	}
	return [string description];
}

static void
soluna_store_marked_text(NSView *view, NSString *text, NSRange selected_range) {
	if (text && text.length > 0) {
		objc_setAssociatedObject(view, kSolunaMarkedTextKey, text, OBJC_ASSOCIATION_COPY_NONATOMIC);
		NSValue *value = [NSValue valueWithRange:selected_range];
		objc_setAssociatedObject(view, kSolunaSelectedRangeKey, value, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	} else {
		objc_setAssociatedObject(view, kSolunaMarkedTextKey, nil, OBJC_ASSOCIATION_ASSIGN);
		objc_setAssociatedObject(view, kSolunaSelectedRangeKey, nil, OBJC_ASSOCIATION_ASSIGN);
	}
}

static NSString *
soluna_current_marked_text(NSView *view) {
	return objc_getAssociatedObject(view, kSolunaMarkedTextKey);
}

static bool
soluna_view_has_marked_text(NSView *view) {
	NSString *text = soluna_current_marked_text(view);
	return text && text.length > 0;
}

static NSRange
soluna_current_selected_range(NSView *view) {
	NSValue *value = objc_getAssociatedObject(view, kSolunaSelectedRangeKey);
	if (value == nil) {
		return NSMakeRange(NSNotFound, 0);
	}
	return [value rangeValue];
}

static void
soluna_set_event_consumed(NSView *view, bool consumed) {
	if (consumed) {
		objc_setAssociatedObject(view, kSolunaConsumedFlagKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
	} else {
		objc_setAssociatedObject(view, kSolunaConsumedFlagKey, nil, OBJC_ASSOCIATION_ASSIGN);
	}
}

static bool
soluna_event_consumed(NSView *view) {
	NSNumber *flag = objc_getAssociatedObject(view, kSolunaConsumedFlagKey);
	return flag && [flag boolValue];
}

static void
soluna_emit_char(uint32_t codepoint, uint32_t modifiers, bool repeat) {
	sapp_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = SAPP_EVENTTYPE_CHAR;
	ev.frame_count = sapp_frame_count();
	ev.char_code = codepoint;
	ev.modifiers = modifiers;
	ev.key_repeat = repeat;
	app_event(&ev);
}

static void
soluna_emit_nsstring(NSString *text) {
	if (text == nil || text.length == 0) {
		return;
	}
	uint32_t mods = soluna_modifiers_from_event([NSApp currentEvent]);
	[text enumerateSubstringsInRange:NSMakeRange(0, text.length)
		options:NSStringEnumerationByComposedCharacterSequences
		usingBlock:^(NSString * _Nullable substring, NSRange substringRange, NSRange enclosingRange, BOOL * _Nonnull stop) {
			uint32_t codepoint = soluna_utf32_from_substring(substring);
			if (codepoint != 0) {
				soluna_emit_char(codepoint, mods, false);
			}
		}];
}

static NSRect
soluna_current_caret_screen_rect(NSView *view) {
	NSRect caret = NSMakeRect(0, 0, 1, 1);
	if (g_soluna_ime_rect.valid) {
		CGFloat dpi_scale = sapp_dpi_scale();
		if (dpi_scale <= 0.0f) {
			dpi_scale = 1.0f;
		}
		CGFloat logical_height = (CGFloat)sapp_height() / dpi_scale;
		CGFloat caret_y = logical_height - (g_soluna_ime_rect.y + g_soluna_ime_rect.h);
		caret = NSMakeRect(g_soluna_ime_rect.x, caret_y, g_soluna_ime_rect.w, g_soluna_ime_rect.h);
	}
	caret = [view convertRect:caret toView:nil];
	if (view.window) {
		caret = [view.window convertRectToScreen:caret];
	}
	return caret;
}

@interface _sapp_macos_view (SolunaIME) <NSTextInputClient>
- (void)soluna_keyDown:(NSEvent *)event;
@end

@implementation _sapp_macos_view (SolunaIME)

- (void)soluna_keyDown:(NSEvent *)event {
	soluna_set_event_consumed(self, false);
	BOOL handled = [[self inputContext] handleEvent:event];
	bool consumed = soluna_event_consumed(self);
	bool hasMarked = soluna_view_has_marked_text(self);
	bool swallow_now = handled && (consumed || hasMarked);
	if (swallow_now || g_soluna_macos_composition) {
		g_soluna_macos_composition = hasMarked;
		return;
	}
	g_soluna_macos_composition = false;
	[self soluna_keyDown:event];
	if (handled && (consumed || hasMarked)) {
		return;
	}
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
	NSString *plain = soluna_plain_string(string);
	soluna_store_marked_text(self, nil, NSMakeRange(NSNotFound, 0));
	if (plain.length > 0) {
		soluna_emit_nsstring(plain);
		soluna_set_event_consumed(self, true);
	} else {
		soluna_set_event_consumed(self, false);
	}
	g_soluna_macos_composition = false;
	soluna_macos_hide_ime_label();
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
	NSString *plain = soluna_plain_string(string);
	soluna_store_marked_text(self, plain, selectedRange);
	g_soluna_macos_composition = (string != nil);
	soluna_macos_position_ime_label(self, plain, selectedRange);
}

- (void)unmarkText {
	soluna_store_marked_text(self, nil, NSMakeRange(NSNotFound, 0));
	g_soluna_macos_composition = false;
	soluna_macos_hide_ime_label();
}

- (NSRange)selectedRange {
	NSRange range = soluna_current_selected_range(self);
	if (range.location == NSNotFound) {
		return NSMakeRange(0, 0);
	}
	return range;
}

- (NSRange)markedRange {
	NSString *text = soluna_current_marked_text(self);
	if (text && text.length > 0) {
		return NSMakeRange(0, text.length);
	}
	return NSMakeRange(NSNotFound, 0);
}

- (BOOL)hasMarkedText {
	NSString *text = soluna_current_marked_text(self);
	return text && text.length > 0;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
	return @[];
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
	NSString *text = soluna_current_marked_text(self);
	if (text == nil) {
		return nil;
	}
	if (range.location == NSNotFound) {
		return nil;
	}
	NSUInteger end = range.location + range.length;
	if (end > text.length) {
		return nil;
	}
	NSString *substr = [text substringWithRange:range];
	if (actualRange) {
		*actualRange = range;
	}
	return [[[NSAttributedString alloc] initWithString:substr] autorelease];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
	(void)point;
	return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
	if (actualRange) {
		*actualRange = range;
	}
	return soluna_current_caret_screen_rect(self);
}

- (void)doCommandBySelector:(SEL)selector {
	id nextResponder = [self nextResponder];
	if ([nextResponder respondsToSelector:@selector(doCommandBySelector:)]) {
		[nextResponder doCommandBySelector:selector];
	}
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

@end

static void
soluna_macos_install_ime(void) {
	static bool installed = false;
	if (installed) {
		return;
	}
	Class viewCls = NSClassFromString(@"_sapp_macos_view");
	if (!viewCls) {
		return;
	}
	Method original = class_getInstanceMethod(viewCls, @selector(keyDown:));
	Method replacement = class_getInstanceMethod(viewCls, @selector(soluna_keyDown:));
	if (original && replacement) {
		method_exchangeImplementations(original, replacement);
	}
	installed = true;
}
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
static WNDPROC g_soluna_prev_wndproc = NULL;
static BOOL g_soluna_wndproc_installed = FALSE;
static LOGFONTW g_soluna_ime_font;
static BOOL g_soluna_ime_font_valid = FALSE;
static BOOL g_soluna_composition = FALSE;

static void
soluna_win32_apply_ime_rect(void) {
    HWND hwnd = (HWND)sapp_win32_get_hwnd();
    if (!hwnd) {
        return;
    }
    HIMC imc = ImmGetContext(hwnd);
    if (!imc) {
        fprintf(stderr, "ImmGetContext failed\n");
        return;
    }
    if (g_soluna_ime_rect.valid) {
        float scale = sapp_dpi_scale();
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
        float rect_top = g_soluna_ime_rect.y;
        if (rect_top < 0.0f) {
            rect_top = 0.0f;
        }
        float rect_height = (g_soluna_ime_rect.h > 0.0f ? g_soluna_ime_rect.h : 1.0f);
        float win_height = (float)sapp_height();
        float rect_bottom = rect_top + rect_height;
        if (win_height > 0.0f && rect_bottom > win_height) {
            rect_bottom = win_height;
        }
        float actual_height = rect_bottom - rect_top;
        if (actual_height <= 0.0f) {
            actual_height = 1.0f;
        }

        LONG caret_x = (LONG)(g_soluna_ime_rect.x * scale + 0.5f);
        LONG caret_y = (LONG)(rect_top * scale + 0.5f);
        LONG caret_w = (LONG)((g_soluna_ime_rect.w > 0.0f ? g_soluna_ime_rect.w : 1.0f) * scale + 0.5f);
        LONG caret_h = (LONG)(actual_height * scale + 0.5f);

        COMPOSITIONFORM cf;
        memset(&cf, 0, sizeof(cf));
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = caret_x;
        cf.ptCurrentPos.y = caret_y;
        ImmSetCompositionWindow(imc, &cf);

        RECT exclude_rect;
        exclude_rect.left = caret_x;
        exclude_rect.top = caret_y;
        exclude_rect.right = caret_x + caret_w;
        exclude_rect.bottom = caret_y + caret_h;

        // convert to screen coordinates for exclusion rectangle
        MapWindowPoints(hwnd, NULL, (LPPOINT)&exclude_rect, 2);

        CANDIDATEFORM cand;
        memset(&cand, 0, sizeof(cand));
        cand.dwIndex = 0;
        cand.dwStyle = CFS_EXCLUDE;
        cand.rcArea = exclude_rect;
        cand.ptCurrentPos.x = exclude_rect.left;
        cand.ptCurrentPos.y = exclude_rect.bottom;
        ImmSetCandidateWindow(imc, &cand);

        if (g_soluna_ime_font_valid) {
            LOGFONTW lf = g_soluna_ime_font;
            if (lf.lfHeight == 0) {
                lf.lfHeight = -(LONG)caret_h;
            }
            ImmSetCompositionFontW(imc, &lf);
        }
    }
	ImmReleaseContext(hwnd, imc);
}

static LRESULT CALLBACK
soluna_win32_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_IME_COMPOSITION:
	case WM_IME_STARTCOMPOSITION:
		g_soluna_composition = TRUE;
		if (g_soluna_ime_rect.valid) {
			soluna_win32_apply_ime_rect();
		}
		break;
	case WM_IME_ENDCOMPOSITION:
		g_soluna_composition = FALSE;
		break;
	case WM_DESTROY:
		g_soluna_composition = FALSE;
		if (g_soluna_prev_wndproc) {
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_soluna_prev_wndproc);
			g_soluna_prev_wndproc = NULL;
			g_soluna_wndproc_installed = FALSE;
		}
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (g_soluna_composition)
			return TRUE;
	default:
		break;
	}
	if (g_soluna_prev_wndproc) {
		return CallWindowProc(g_soluna_prev_wndproc, hwnd, msg, wParam, lParam);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void
soluna_win32_install_wndproc(void) {
	if (g_soluna_wndproc_installed) {
		return;
	}
	HWND hwnd = (HWND)sapp_win32_get_hwnd();
	if (!hwnd) {
		return;
	}
	WNDPROC prev = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)soluna_win32_wndproc);
	if (prev) {
		g_soluna_prev_wndproc = prev;
		g_soluna_wndproc_installed = TRUE;
	}
}

static void
soluna_win32_set_ime_font(const char *font_name, float height_px) {
	float scale = sapp_dpi_scale();
	if (scale <= 0.0f) {
		scale = 1.0f;
	}
	LOGFONTW lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = CLEARTYPE_QUALITY;
	if (height_px > 0.0f) {
		lf.lfHeight = -(LONG)(height_px * scale + 0.5f);
	}
	if (font_name && font_name[0]) {
		int wlen = MultiByteToWideChar(CP_UTF8, 0, font_name, -1, NULL, 0);
		if (wlen > 0 && wlen <= (int)(sizeof(lf.lfFaceName) / sizeof(wchar_t))) {
			MultiByteToWideChar(CP_UTF8, 0, font_name, -1, lf.lfFaceName, wlen);
		}
	}
	g_soluna_ime_font = lf;
	g_soluna_ime_font_valid = TRUE;
}
#endif

static int
lmessage_send(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	void (*send_message)(void *ud, void *p) = lua_touserdata(L, 1);
	void *send_message_ud = lua_touserdata(L, 2);
	const char * what = NULL;
	if (lua_type(L, 3) == LUA_TSTRING) {
		what = lua_tostring(L, 3);
	} else {
		luaL_checktype(L, 3, LUA_TLIGHTUSERDATA);
		what = (const char *)lua_touserdata(L, 3);
	}
	int64_t p1 = luaL_optinteger(L, 4, 0);
	struct soluna_message * msg = NULL;
	if (lua_isnoneornil(L, 5)) {
		msg = message_create64(what, p1);
	} else {
		int p2 = luaL_checkinteger(L, 5);
		msg = message_create(what, (int)p1, p2);
	}
	send_message(send_message_ud, msg);
	return 0;
}

static int
lmessage_unpack(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	struct soluna_message *m = (struct soluna_message *)lua_touserdata(L,1);
	lua_pushstring(L, m->type);
	lua_pushinteger(L, m->v.p[0]);
	lua_pushinteger(L, m->v.p[1]);
	lua_pushinteger(L, m->v.u64);
	message_release(m);
	return 4;
}

static int
lquit_signal(lua_State *L) {
	if (CTX) {
		CTX->quitL = CTX->L;
		CTX->L = NULL;
	}
	return 0;
}

static int
levent_unpack(lua_State *L) {
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	struct event_message em;
	app_event_unpack(&em, lua_touserdata(L, 1));
	lua_pushlightuserdata(L, (void *)em.typestr);
	lua_pushinteger(L, em.p1);
	lua_pushinteger(L, em.p2);
	return 3;
}

static int
lset_window_title(lua_State *L) {
	if (CTX == NULL || lua_type(L, 1) != LUA_TSTRING)
		return 0;
	const char * text = lua_tostring(L, 1);
	sapp_set_window_title(text);
	return 0;
}

static int
lset_ime_rect(lua_State *L) {
#if defined(__APPLE__) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
	if (lua_isnoneornil(L, 1)) {
		g_soluna_ime_rect.valid = false;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
		soluna_win32_apply_ime_rect();
#elif defined(__APPLE__)
		soluna_macos_hide_ime_label();
#endif
		return 0;
	}
	g_soluna_ime_rect.x = (float)luaL_checknumber(L, 1);
	g_soluna_ime_rect.y = (float)luaL_checknumber(L, 2);
	g_soluna_ime_rect.w = (float)luaL_checknumber(L, 3);
	g_soluna_ime_rect.h = (float)luaL_checknumber(L, 4);
	g_soluna_ime_rect.valid = true;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
	soluna_win32_apply_ime_rect();
#elif defined(__APPLE__)
	if (g_soluna_ime_label && ![g_soluna_ime_label isHidden]) {
		NSView *view = [g_soluna_ime_label superview];
		if (view) {
			soluna_macos_refresh_ime_label(view);
		}
	}
#endif
#endif
	return 0;
}

static int
lset_ime_font(lua_State *L) {
	const char *name = NULL;
	float size = 0.0f;
	int top = lua_gettop(L);
	if (top == 0) {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
		g_soluna_ime_font_valid = FALSE;
#endif
#if defined(__APPLE__)
		soluna_macos_set_ime_font(NULL, 0.0f);
#endif
		return 0;
	}
	if (top == 1) {
		size = (float)luaL_checknumber(L, 1);
	} else {
		if (!lua_isnoneornil(L, 1)) {
			if (lua_type(L, 1) != LUA_TSTRING) {
				return luaL_error(L, "set_ime_font expects string font name");
			}
			name = lua_tostring(L, 1);
		}
		size = (float)luaL_checknumber(L, 2);
	}
	if (size < 0.0f) {
		size = 0.0f;
	}
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
	soluna_win32_set_ime_font(name, size);
#endif
#if defined(__APPLE__)
	soluna_macos_set_ime_font(name, size);
#endif
	return 0;
}

static int
lclose_window(lua_State *L) {
	sapp_quit();
	return 0;
}

static int
lmqueue(lua_State *L) {
	if (CTX == NULL || CTX->mqueue == NULL) {
		return luaL_error(L, "Not init mqueue");
	}
	lua_pushlightuserdata(L, CTX->mqueue);
	return 1;
}

static int
lcontext_acquire(lua_State *L) {
#if defined(__linux__)
  _sapp_glx_make_current();
#endif
  return 0;
}

static int
lcontext_release(lua_State *L) {
#if defined(__linux__)
  _sapp.glx.MakeCurrent(_sapp.x11.display, None, NULL);
#endif
  return 0;
}

static int
lversion(lua_State *L) {
	lua_pushinteger(L, SOLUNA_API_VERSION);
	lua_pushstring(L, SOLUNA_HASH_VERSION);
	return 2;
}

int
luaopen_soluna_app(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "context_acquire", lcontext_acquire },
		{ "context_release", lcontext_release },
		{ "mqueue", lmqueue },
		{ "unpackmessage", lmessage_unpack },
		{ "sendmessage", lmessage_send },
		{ "unpackevent", levent_unpack },
		{ "set_window_title", lset_window_title },
		{ "set_ime_rect", lset_ime_rect },
		{ "set_ime_font", lset_ime_font },
		{ "quit", lquit_signal },
		{ "close_window", lclose_window },
		{ "platform", NULL },
		{ "version", lversion },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);
	lua_pushliteral(L, PLATFORM);
	lua_setfield(L, -2, "platform");

	return 1;
}

static void
log_func(const char* tag, uint32_t log_level, uint32_t log_item, const char* message, uint32_t line_nr, const char* filename, void* user_data) {
	if (CTX == NULL || CTX->send_log == NULL) {
		fprintf(stderr, "%s (%d) : %s\n", filename, line_nr, message);
		return;
	}
	struct log_info *msg = (struct log_info *)malloc(sizeof(*msg));
	if (tag) {
		strncpy(msg->tag, tag, sizeof(msg->tag));
		msg->tag[sizeof(msg->tag)-1] = 0;
	} else {
		msg->tag[0] = 0;
	}
	msg->log_level = log_level;
	msg->log_item = log_item;
	msg->line_nr = line_nr;
	if (message) {
		strncpy(msg->message, message, sizeof(msg->message));
		msg->message[sizeof(msg->message)-1] = 0;
	} else {
		msg->message[0] = 0;
	}
	msg->filename = filename;
	CTX->send_log(CTX->send_log_ud, 0, msg, sizeof(*msg));
}

void soluna_openlibs(lua_State *L);

static const char *code = "local embed = require 'soluna.embedsource' ; local f = load(embed.runtime.main()) ; return f(...)";

static void
set_app_info(lua_State *L, int index) {
	lua_newtable(L);
	lua_pushinteger(L, sapp_width());
	lua_setfield(L, -2, "width");
	lua_pushinteger(L, sapp_height());
	lua_setfield(L, -2, "height");
	lua_setfield(L, index, "app");
}

static int
pmain(lua_State *L) {
	soluna_openlibs(L);
	int n = sargs_num_args();
	luaL_checkstack(L, n+1, NULL);
	int i;
	lua_newtable(L);
	int arg_table = lua_gettop(L);
	for (i=0;i<n;i++) {
		const char *k = sargs_key_at(i);
		const char *v = sargs_value_at(i);
		if (v[0] == 0) {
			lua_pushstring(L, sargs_key_at(i));
		} else {
			lua_pushstring(L, v);
			lua_setfield(L, arg_table, k);
		}
	}
	set_app_info(L, arg_table);
	
	int arg_n = lua_gettop(L) - arg_table + 1;
	if (luaL_loadstring(L, code) != LUA_OK) {
		return lua_error(L);
	}
	lua_insert(L, -arg_n-1);
	lua_call(L, arg_n, 1);
	return 1;
}

static void *
get_ud(lua_State *L, const char *key) {
	if (lua_getfield(L, -1, key) != LUA_TLIGHTUSERDATA) {
		lua_pop(L, 1);
		return NULL;
	}
	void * ud = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return ud;
}

static int
get_function(lua_State *L, const char *key, int index) {
	if (lua_getfield(L, -1, key) != LUA_TFUNCTION) {
		fprintf(stderr, "main.lua need return a %s function", key);
		return 1;
	}
	lua_insert(L, index);
	return 0;
}

static int
init_callback(lua_State *L, struct app_context * ctx) {
	if (!lua_istable(L, -1)) {
		fprintf(stderr, "main.lua need return a table, it's %s\n", lua_typename(L, lua_type(L, -1)));
		return 1;
	}
	ctx->send_log = get_ud(L, "send_log");
	ctx->send_log_ud = get_ud(L, "send_log_ud");
	ctx->mqueue = get_ud(L, "mqueue");
	if (get_function(L, "frame", FRAME_CALLBACK))
		return 1;
	if (get_function(L, "cleanup", CLEANUP_CALLBACK))
		return 1;
	if (get_function(L, "event", EVENT_CALLBACK))
		return 1;
	lua_settop(L, CALLBACK_COUNT);
	return 0;
}

static int
msghandler(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	luaL_traceback(L, L, msg, 1);
	return 1;
}

static int
start_app(lua_State *L) {
	lua_settop(L, 0);
	lua_pushcfunction(L, msghandler);
	lua_pushcfunction(L, pmain);
	if (lua_pcall(L, 0, 1, 1) != LUA_OK) {
		fprintf(stderr, "Start fatal : %s", lua_tostring(L, -1));
		return 1;
	} else {
		return init_callback(L, CTX);
	}
}

static void
app_init() {
	static struct app_context app;
	lua_State *L = luaL_newstate();
	if (L == NULL)
		return;

	app.L = L;
	app.quitL = NULL;
	app.send_log = NULL;
	app.send_log_ud = NULL;
	app.mqueue = NULL;
	
	CTX = &app;

#if defined(__APPLE__)
	soluna_macos_install_ime();
#endif
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
	soluna_win32_install_wndproc();
#endif
	
	sg_setup(&(sg_desc) {
        .environment = sglue_environment(),
        .logger.func = log_func,			
	});
	if (start_app(L)) {
		sargs_shutdown();
		lua_close(L);
		app.L = NULL;
		sapp_quit();
	} else {
		sargs_shutdown();
	}
}

static lua_State *
get_L(struct app_context *ctx) {
	if (ctx == NULL)
		return NULL;
	lua_State *L = ctx->L;
	if (L == NULL) {
		if (ctx->quitL != NULL) {
			ctx->L = ctx->quitL;
			ctx->quitL = NULL;
			sapp_quit();
			return NULL;
		}
	}
	return L;
}

static void
invoke_callback(lua_State *L, int index, int nargs) {
	lua_pushvalue(L, index);
	if (nargs > 0) {
		lua_insert(L, -nargs-1);
	}
	if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error : %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}

static void
app_frame() {
	lua_State *L = get_L(CTX);
	if (L) {
		lua_pushinteger(L, sapp_frame_count());
		invoke_callback(L, FRAME_CALLBACK, 1);
	}
}

static void
app_cleanup() {
	lua_State *L = get_L(CTX);
	if (L) {
		invoke_callback(L, CLEANUP_CALLBACK, 0);
	}
	sg_shutdown();
}

static void
app_event(const sapp_event* ev) {
#if defined(__APPLE__)
	if (g_soluna_macos_composition &&
		(ev->type == SAPP_EVENTTYPE_KEY_DOWN ||
		 ev->type == SAPP_EVENTTYPE_KEY_UP)) {
		return;
	}
#endif
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
	if (ev->type == SAPP_EVENTTYPE_FOCUSED && g_soluna_ime_rect.valid) {
		soluna_win32_apply_ime_rect();
	}
#endif
	lua_State *L = get_L(CTX);
	if (L) {
		lua_pushlightuserdata(L, (void *)ev);
		invoke_callback(L, EVENT_CALLBACK, 1);
	}
}

sapp_desc
sokol_main(int argc, char* argv[]) {
	sargs_desc arg_desc;
	memset(&arg_desc, 0, sizeof(arg_desc));
	arg_desc.argc = argc;
	arg_desc.argv = argv;
	sargs_setup(&arg_desc);
	
	sapp_desc d;
	memset(&d, 0, sizeof(d));
		
	d.width = 1024;
	d.height = 768;
	d.init_cb = app_init;
	d.frame_cb = app_frame;
	d.cleanup_cb = app_cleanup;
	d.event_cb = app_event;
	d.logger.func = log_func;
	d.win32_console_utf8 = 1;
	d.win32_console_attach = 1;
	d.window_title = "soluna";
	d.alpha = 0;

	return d;
}
