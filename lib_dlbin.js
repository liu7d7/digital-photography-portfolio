let dlbin_api = {
  download_binary_file__deps: ['malloc'],
  download_binary_file: function(url, callback, userData) {
    let dispatch_callback = res => {
      {{{makeDynCall('vpip', 'callback')}}}(res.size && res.data, res.size, userData);
    };

    fetch(utf8(url)).then(res=>res.blob()).then(b=>b.arrayBuffer()).then(b => {
      let d = new Uint8Array(b);
      const dst = _malloc(d.length);
      HEAPU8.set(d, {{{ shiftPtr('dst', 0) }}});
      dispatch_callback({data: {{{ shiftPtr('dst', 0) }}}, size: d.length});
    }).catch(dispatch_callback)
  }
}

mergeInto(LibraryManager.library, dlbin_api)
