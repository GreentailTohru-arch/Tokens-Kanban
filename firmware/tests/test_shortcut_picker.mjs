import assert from 'node:assert/strict';
import {normalizeShortcut,selectKey} from '../companion/postcard/shortcut-picker.js';
assert.equal(normalizeShortcut('LeftAlt+T'),'Alt+T');assert.equal(selectKey('','Alt'),'Alt');assert.equal(selectKey('Alt','T'),'Alt+T');assert.equal(selectKey('Alt+T','T'),'Alt');assert.equal(selectKey('Alt+T','A'),'Alt+A');assert.equal(selectKey('T','Ctrl'),'Ctrl+T');assert.equal(selectKey('Ctrl+Shift+Alt+T','Win'),null);assert.equal(selectKey('Alt','RightAlt'),'Alt+RightAlt');console.log('Keyboard picker selection and limits PASS');
