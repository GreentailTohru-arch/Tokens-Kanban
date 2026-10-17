"""Navigate through the real sidebar toggle, including collapsed mobile drawers."""
def navigate(page, target):
    if page.locator('#status').is_visible():
        page.locator('#status .notice-close').click()
        page.locator('#status').wait_for(state='hidden')
    page.evaluate("() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)))")
    if page.locator('#menuToggle').get_attribute('aria-expanded') != 'true':
        page.locator('#menuToggle').click()
    page.locator('#'+target).click()

def set_web_mode(page, mode):
    if page.locator('html').get_attribute('data-web-mode') != mode:
        page.locator('#webModeToggle').click()
