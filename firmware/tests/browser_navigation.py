"""Navigate through the real sidebar toggle, including collapsed mobile drawers."""
def navigate(page, target):
    page.evaluate("() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)))")
    if page.locator('#menuToggle').get_attribute('aria-expanded') != 'true':
        page.locator('#menuToggle').click()
    page.locator('#'+target).click()
