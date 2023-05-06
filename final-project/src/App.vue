<template>
  <div>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/css/bootstrap.min.css">
    <header>
      <!-- Navbar -->
      <MDBNavbar expand="lg" light bg="white" container>
        <MDBNavbarToggler
          id="hamburger"
          aria-controls="navbarExample01"
          aria-expanded="false"
          aria-label="Toggle navigation"
          v-model="isOpen"
          @click="toggleSideNav"
        >
        </MDBNavbarToggler>
        <h1 id="title-title">KOAT</h1>
        <MDBNavbarNav v-if="isOpen" class="mb-2 mb-lg-0">
          <MDBNavbarItem to="/" exact>
            Home
          </MDBNavbarItem>
          <MDBNavbarItem to="/Find">
            Find
          </MDBNavbarItem>
          <MDBNavbarItem to="/Track">
            Track
          </MDBNavbarItem>
        </MDBNavbarNav>
      </MDBNavbar>
      <!-- Navbar -->

      <!-- Sidenav -->
      <MDBSideNav
        fixed
        slim
        :opened="isOpen"
        backdrop={true}
        breakWidth={960}
        maskTransparent={true}
        bg="white"
        textColor="black"
      >
        <MDBSideNavNav>
          <MDBSideNavItem to="/" exact>
            Home
          </MDBSideNavItem>
          <MDBSideNavItem to="/Find">
            Find
          </MDBSideNavItem>
          <MDBSideNavItem to="/Track">
            Track
          </MDBSideNavItem>
        </MDBSideNavNav>
      </MDBSideNav>
      <!-- Sidenav -->
    </header>
    <body id="app">
      <RouterView />
    </body>
  </div>
</template>


<script setup>
//http://3.101.68.31:5000/connection delay for one second if false then I will send a notification
//if the connection state hasn't changed then do not do anything until it has changed
import {
  MDBNavbar,
  MDBNavbarToggler,
  MDBNavbarNav,
  MDBNavbarItem,
  MDBSideNav,
  MDBSideNavNav,
  MDBSideNavItem
} from "mdb-vue-ui-kit";

import { ref } from "vue";

const isOpen = ref(false);

function toggleSideNav() {
  isOpen.value = !isOpen.value;
}
</script>

<style scoped>
#hamburger {
  display: inline-block;
  margin-right: 15px;
  background-image: url(data:image/jpeg;base64,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);
  background-size: 40px;
  width: 40px;
  height: 40px;
}

#title-title {
  display: inline-block;
  text-align: center;
  width: 100%;
}


#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}
</style>
