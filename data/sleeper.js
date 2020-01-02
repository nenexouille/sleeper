var app = new Vue ({
  el: "#sleeper",
  data: {
    items: [],
    weekday: ['dimanche','lundi','mardi','mercredi','jeudi','vendredi','samedi']
  },
  
  mounted() {
    axios.get("planning.json")
    .then(response => {this.items = response.data})
  },

  methods: {
    invert: function(idxday, idxhour, idxminute) {
      this.items[idxday][idxhour][idxminute] = !this.items[idxday][idxhour][idxminute]
      Vue.set(app.items, idxday, this.items[idxday])
    },

    send: function() {
      console.log(JSON.stringify(this.items));
      axios({
        method: "post",
        url: "updatePlanning?planning="+JSON.stringify(this.items),
        data: this.items
      });
      
    }
  }

})